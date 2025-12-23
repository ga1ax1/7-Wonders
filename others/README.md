# Seven Wonders Duel - C++ Implementation Design Document

## 1. 架构概览 (Architecture Overview)

本项目严格遵循“MVC (Model-View-Controller)”架构模式，并结合了“策略模式 (Strategy Pattern)”与“工厂模式 (Factory Pattern)”来处理复杂的游戏逻辑与数据构建。

### 核心分层 (Layers)

1.  **Model Layer (`Model.cpp/h`)**:
    *   **职责**: 维护游戏的所有状态数据（玩家、版图、卡牌堆叠、军事轨道）。
    *   **特点**: 或者是“贫血模型”或者是“充血模型”。在这里我们选择了**充血模型**，即 `Player` 和 `Board` 类中包含大量业务逻辑（如 `calculateCost`, `applyShields`），保证数据的一致性。
    *   **关键类**: `GameModel`, `Player`, `Board`, `MilitaryTrack`, `CardPyramid`.

2.  **View Layer (`View.cpp/h`)**:
    *   **职责**: 负责将 Model 的状态可视化（当前为控制台输出），以及定义 Agent 接口（输入源）。
    *   **设计**: 通过 `GameView` 类封装渲染逻辑，使得 Controller 只需调用 `renderGameState()` 而无需关心具体的 IO 实现。Agent 接口 (`IPlayerAgent`) 使得人类玩家和 AI 玩家可以无缝切换。

3.  **Controller Layer (`Controller.cpp/h`)**:
    *   **职责**: 游戏的主循环、状态机流转、动作分发。
    *   **设计**: `GameController` 是系统的指挥官。它持有 Model 和 View，通过 `runGameLoop` 驱动游戏，接收 Agent 的 `Action`，验证后更新 Model，并处理副作用（如触发状态跳转）。

4.  **Engine/Strategy Layer (`Engine.cpp/h`)**:
    *   **职责**: 处理卡牌和奇迹的多样化效果。
    *   **设计**: 这是一个纯逻辑层，解耦了“卡牌数据”与“卡牌行为”。

5.  **Utils/Data Layer (`Utils.cpp`, `Card.h`, `Global.h`)**:
    *   **职责**: 定义基础数据结构、全局常量以及数据加载逻辑。

---

## 2. 设计模式详解 (Design Patterns)

本项目刻意使用了多种设计模式，以应对《七大奇迹对决》复杂的规则体系。

### 2.1 策略模式 (Strategy Pattern) - `IEffect`
*   **应用场景**: 卡牌和奇迹拥有各种各样的效果（产资源、给盾牌、给分、再来一回合、毁灭对手卡牌等）。
*   **为什么选择它**: 
    *   如果不使用策略模式，我们可能需要在 `Player::addCard` 中写巨大的 `switch-case` 语句，或者使用多重继承（如 `MilitaryCard`, `ScienceCard`）。
    *   **继承的缺陷**: 多重继承会导致“钻石继承”问题（如一张卡既给钱又给分），且难以扩展。
    *   **策略模式的优势**: 将“效果”封装为独立的 `IEffect` 对象。一张 `Card` 持有一个 `vector<shared_ptr<IEffect>>`。这是一种**组合 (Composition)** 优于 **继承 (Inheritance)** 的设计。我们可以轻松创建一张“既产木头又给盾牌”的卡，只需塞入两个 Effect 对象即可，无需修改类结构。

### 2.2 工厂模式 (Factory Pattern) - `IDataLoader`
*   **应用场景**: 游戏中有上百张不同的卡牌，每张卡牌的属性（名字、造价、效果）各不相同。
*   **为什么选择它**:
    *   我们不希望在 C++ 代码中硬编码 `new Card("LumberYard", ...)`。
    *   `IDataLoader` 充当抽象工厂，负责从数据源（虽然目前是 Mock 的，未来可以是 JSON/XML）读取描述，并组装出具体的 `Card` 对象（挂载正确的 `IEffect`）。这实现了数据构建与使用的分离。

### 2.3 状态模式 (State Machine) - `GameState`
*   **应用场景**: 游戏流程不仅仅是“回合制”。还有“奇迹轮抽”、“选Token中断”、“毁灭卡牌中断”等特殊阶段。
*   **为什么选择它**:
    *   在 `GameController` 中维护一个 `GameState` 枚举。
    *   每次处理动作后，根据效果触发状态跳转（例如 `LibraryEffect` 将状态设为 `WAITING_FOR_TOKEN_SELECTION_LIB`）。
    *   主循环 `processTurn` 根据当前状态决定调用哪个处理函数 (`handleWonderDraft`, `handlePlayPhase`, `triggerIdling`)。这比一堆布尔标志位 (`isDrafting`, `isWaitingForPyramid`) 要清晰得多且不易出错。

### 2.4 组合模式 (Composite Pattern) - `Card` 与 `CardPyramid`
*   **应用场景**: 卡牌的层级结构（金字塔）。
*   **实现**: 虽然没有使用标准的 Composite 类，但 `CardPyramid` 内部构建了一个 **有向无环图 (DAG)**。
*   **为什么**: 卡牌的遮盖关系本质上是图的依赖。我们需要处理“移除一张卡 -> 检查依赖 -> 翻开下层卡”的连锁反应。

## 3. 详细设计决策 (Detailed Design Decisions)

### 3.1 为什么避免 `Card` 的继承体系？
*   在初学者设计中，常会出现 `class MilitaryCard : public Card`。
*   但在 7 Wonders Duel 中，卡牌类型（颜色）更多是一种**标签 (Tag)**，用于判定合法性（如宙斯拆红卡）或计分（黄卡数 * 2）。
*   卡牌的**行为**通过 `IEffect` 定义。
*   因此，`Card` 类被设计为一个**容器**，包含 `CardType` 枚举和 `IEffect` 列表。这样极大地提高了系统的灵活性。

### 3.2 智能指针的使用 (`std::shared_ptr`, `std::unique_ptr`)
*   `GameModel` 独占拥有 `Player` 和 `Board` (`unique_ptr`)。
*   `Card` 持有 `Effect` 的共享所有权 (`shared_ptr`)，这允许 flyweight 模式（虽然未完全实现），即多个相同的卡牌可以共享同一个 Effect 实例以节省内存。
*   `Board` 和 `Player` 持有 `Card*` 裸指针。这是因卡牌的所有权通常属于“游戏及其数据池 (`GameController::cardPools`)”，`Board` 和 `Player` 只是引用它们。避免了复杂的生命周期管理问题。

### 3.3 物理引擎式的 `MilitaryTrack`
*   我们将军事轨道封装为独立对象，其 `applyShields` 方法不仅移动位置，还内嵌了“掠夺触发”逻辑。这符合高内聚原则：军事相关的所有逻辑（移动、扣钱、胜利判定）都由该类自治，Controller 只需调用一次接口。
