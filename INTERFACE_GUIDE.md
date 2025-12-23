# Seven Wonders Duel: 技术架构与类参考规范（仅参考，分析时建议重新详细分析）

## 1. 架构总览 (Architecture Overview)
本项目采用 **领域驱动设计 (Domain-Driven Design)** 思想，结合 **MVC (Model-View-Controller)** 模式。为处理复杂的棋盘游戏规则，架构中深度集成了 **状态模式 (State Pattern)** 和 **命令模式 (Command Pattern)**，实现了逻辑校验与业务执行的物理隔离。

---

## 2. 核心基础设施 (Core Infrastructure)

### 2.1 Action (结构体)
*   **说明**: 描述玩家意图的原子数据包。
*   **关键属性**:
    *   `ActionType type`: 动作类型（如 `BUILD_CARD`）。
    *   `string targetCardId`: 目标卡牌唯一标识。
    *   `string targetWonderId`: 目标奇迹标识（可选）。
    *   `ProgressToken selectedToken`: 选中的科技标记（可选）。

### 2.2 ActionResult (结构体)
*   **说明**: 逻辑校验层的反馈结果。
*   **关键属性**:
    *   `bool isValid`: 动作是否允许执行。
    *   `int cost`: 执行此动作玩家需支付的实际金币（由 `Player` 逻辑动态计算）。
    *   `string message`: 校验失败时的错误描述。

---

## 3. 控制与逻辑层 (Controller & Logic)

### 3.1 GameController (类)
*   **设计模式**: 外观模式 (Facade), 状态模式上下文 (Context)。
*   **继承关系**: 实现 `ILogger`, `IGameActions` 接口。
*   **主要属性**:
    *   `unique_ptr<GameModel> m_model`: 持有游戏完整状态。
    *   `unique_ptr<IGameStateLogic> m_stateLogic`: 当前状态的逻辑处理器。
    *   `GameState m_currentState`: 状态枚举。
*   **核心方法**:
    *   `ActionResult validateAction(Action)`: 委派给 `m_stateLogic` 进行规则校验。
    *   `bool processAction(Action)`: 校验通过后，通过 `CommandFactory` 创建并执行命令。
    *   `void setState(GameState)`: 切换当前状态并同步更新逻辑处理器。

### 3.2 IGameStateLogic (抽象基类)
*   **设计模式**: 状态模式 (State Pattern)。
*   **子类**: `WonderDraftState`, `AgePlayState`, `TokenSelectionState`, `DestructionState`, `DiscardBuildState`, `StartPlayerSelectionState`, `GameOverState`。
*   **核心方法**:
    *   `virtual ActionResult validate(Action, GameController&)`: 定义该状态下特有的规则准入条件。

---

## 4. 业务执行层 (Command Execution)

### 4.1 IGameCommand (抽象基类)
*   **设计模式**: 命令模式 (Command Pattern)。
*   **子类**: `DraftWonderCommand`, `BuildCardCommand`, `DiscardCardCommand`, `BuildWonderCommand`, `SelectProgressTokenCommand`, `DestructionCommand`, `SelectFromDiscardCommand`, `ChooseStartingPlayerCommand`。
*   **核心方法**:
    *   `virtual void execute(GameController&)`: 封装具体的业务逻辑，如扣除金币、更新棋盘、触发卡牌效果。

### 4.2 CommandFactory (静态类)
*   **设计模式**: 简单工厂 (Simple Factory)。
*   **方法**:
    *   `static unique_ptr<IGameCommand> createCommand(Action)`: 根据动作类型实例化对应的命令对象。

---

## 5. 领域模型层 (Domain Model)

### 5.1 Player (类)
*   **主要属性**:
    *   `int m_coins`: 玩家持有金币。
    *   `map<ResourceType, int> m_fixedResources`: 基础资源产量。
    *   `vector<vector<ResourceType>> m_choiceResources`: 多选一资源产量（如黄卡/奇迹提供）。
    *   `set<string> m_ownedChainTags`: 拥有的连锁标记。
*   **核心方法**:
    *   `pair<bool, int> calculateCost(ResourceCost, Player& opponent, CardType)`: **核心算法**。根据玩家自有资源、多选一资源的最优分配、对手产量（决定交易价格）及科技标记减免，计算出最低金币成本。

### 5.2 Board (类)
*   **主要属性**:
    *   `MilitaryTrack m_militaryTrack`: 军事轨道状态。
    *   `CardPyramid m_cardStructure`: 当前时代的卡牌金字塔拓扑。
*   **核心方法**:
    *   `vector<int> moveMilitary(int shields, int playerId)`: 移动冲突标记并返回触发的掠夺事件。

### 5.3 Card / Wonder (类)
*   **说明**: 游戏基础对象。
*   **属性**: 包含 `ResourceCost`, `IEffect` 列表, 连锁标记等。

---

## 6. 效果系统 (Effect System)

### 6.1 IEffect (抽象基类)
*   **设计模式**: 观察者/触发器模式。
*   **子类**: `ProductionEffect`, `MilitaryEffect`, `ScienceEffect`, `VictoryPointEffect`, `GuildEffect` 等。
*   **核心方法**:
    *   `virtual void apply(Player* self, Player* opp, ILogger*, IGameActions*)`: 执行即时效果（如加钱、移动冲突标记）。
    *   `virtual int calculateScore(Player* self, Player* opp)`: 计算周期性或条件性得分。

---

## 7. 辅助模块 (Utilities)

### 7.1 TinyJson (命名空间)
*   **功能**: 自研轻量级 JSON 解析器。
*   **类**: `Value` (基于 `std::variant` 的类型安全容器), `Parser` (递归下降解析器)。

### 7.2 InputManager (类)
*   **功能**: 人机交互适配器。
*   **方法**: `Action promptHumanAction(...)`: 将控制台字符串解析并验证为 `Action` 对象。

---

## 8. 设计模式总结 (Design Pattern Summary)

1.  **State Pattern**: 解决了多阶段、多中断的游戏流控制，消除庞大的 `switch-case`。
2.  **Command Pattern**: 确保每个玩家动作都是可回溯、原子化的，方便后续扩展悔棋或动作日志功能。
3.  **Strategy Pattern**: 在 `GuildEffect` 中用于处理不同行会卡的计分算法。
4.  **Facade Pattern**: `GameController` 隐藏了复杂的内部系统，为 `main.cpp` 提供简洁的驱动接口。