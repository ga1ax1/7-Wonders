# design Pattern Educational Guide (C++ Case Study)

本指南旨在通过《七大奇迹对决》的 C++ 源码，深入浅出地讲解常用设计模式（Design Patterns）与 SOLID 原则。

---

## 1. 策略模式 (Strategy Pattern)

### 🎓 通俗解释
**“锦囊妙计”**。想象你是一个古代将军，你随身带着几个锦囊。遇到火攻就拆开“火攻锦囊”，遇到水攻就拆开“水攻锦囊”。你（将军）不需要知道锦囊里具体写了什么兵法，你只需要知道在何时使用它们。
*   **锦囊** = 策略接口 (`Strategy Interface`)
*   **火攻/水攻** = 具体策略 (`Concrete Strategy`)
*   **将军** = 上下文 (`Context`)

### 🛠 解决什么问题？
*   **问题**: 所有的逻辑都写在一个巨大的 `if-else` 或 `switch` 语句里。例如：`if (cardType == RED) { ... } else if (cardType == GREEN) { ... }`。这违反了 **OCP (开闭原则)**，因为每次加新类型都要改原有代码。
*   **优势**: 算法可以独立于使用它的客户端变化；易于扩展新的策略。
*   **劣势**: 可能会产生很多策略类（文件数增加）；客户端需要了解不同策略的区别。

### 🌐 现实生活类比
*   **导航 App**: 你去机场，可以选择“打车策略”、“公交策略”或“骑行策略”。App (Context) 只需要算出时间，具体的路径计算交给具体的策略。

### 💻 MVP Code Template
```cpp
// 1. 抽象策略
struct Strategy { virtual void execute() = 0; };

// 2. 具体策略
struct ConcreteStrategyA : Strategy { void execute() override { cout << "Plan A"; } };
struct ConcreteStrategyB : Strategy { void execute() override { cout << "Plan B"; } };

// 3. 上下文
class Context {
    Strategy* strategy;
public:
    void setStrategy(Strategy* s) { strategy = s; }
    void run() { strategy->execute(); }
};
```

### 🔍 本项目 Case Study: `IEffect`
在《七大奇迹对决》中，卡牌效果是千变万化的。
*   **代码位置**: `Engine.h` / `Engine.cpp`
*   **实现**: 
    *   **接口**: `IEffect` 是所有效果的基类（锦囊）。
    *   **策略**: `MilitaryEffect` (给盾牌), `ProductionEffect` (产资源), `ScienceEffect` (给符号)。
    *   **使用**: `Card` 类持有 `vector<IEffect*> effects`。当通过 `onBuild()` 建造卡牌时，它遍历执行所有效果。
*   **SOLID**: 
    *   **OCP (开闭)**: 新增一种“复制对手科技”的效果，只需加一个 `CopyScienceEffect` 类，**不需要修改** `Card` 或 `Controller`。
    *   **SRP (单一职责)**: `MilitaryEffect` 只管打仗，`ProductionEffect` 只管种田。

---

## 2. 状态模式 (State Pattern)

### 🎓 通俗解释
**“变形金刚”**。威震天变形为坦克时，他的行为是“开炮”；变形为飞机时，行为是“飞行”。同一个对象（威震天），在不同状态下，对同一个指令（进攻）有不同的反应。

### 🛠 解决什么问题？
*   **问题**: `GameController` 里充满了大量的 `bool` 标志位：`isDrafting`, `isWaitingForPyramid`, `isDiscarding`。逻辑混乱，容易产生非法状态（比如同时在 `PreGame` 和 `InGame`）。
*   **优势**: 将与特定状态相关的行为局部化到独立的类或枚举逻辑中；消除庞大的条件分支语句。
*   **劣势**: 如果状态很少，使用模式可能因小失大。

### 🌐 现实生活类比
*   **自动售货机**: 
    *   状态: `NoCoin` -> `HasCoin` -> `Sold` -> `SoldOut`.
    *   动作: `InsertCoin`. 在 `NoCoin` 状态下接受硬币并转为 `HasCoin`；在 `SoldOut` 状态下则退币。

### 💻 MVP Code Template
```cpp
// 简化版 (枚举实现，适合轻量级)
enum State { IDLE, RUNNING, PAUSED };
class Machine {
    State state = IDLE;
public:
    void pressButton() {
        switch(state) {
            case IDLE: state = RUNNING; run(); break;
            case RUNNING: state = PAUSED; pause(); break;
            case PAUSED: state = RUNNING; run(); break;
        }
    }
};
```

### 🔍 本项目 Case Study: `GameState`
*   **代码位置**: `Controller.cpp` (`processTurn`, `handleWonderDraft`...)
*   **实现**:
    *   我们使用了枚举 `GameState` 而非完整的 State Class (为了简化代码复杂度)。
    *   状态流转: `WONDER_DRAFT` (奇迹轮抽) -> `AGE_PLAY_PHASE` (打牌) -> `WAITING_FOR_DESTRUCTION` (中断)。
    *   在 `runGameLoop` 中，根据当前 `state` 分发到不同的处理函数。
*   **对比**: 如果没有状态机，当玩家打出“宙斯”卡要求摧毁对手卡牌时，我们必须用 `bool hasJustPlayedZeus = true` 这种临时变量来阻塞下一回合，非常容易出错。

---

## 3. 工厂模式 (Factory Pattern)

### 🎓 通俗解释
**“披萨店点餐”**。你饿了，想吃披萨。你不需要自己去买面粉、切番茄、烤箱设定温度。你只需要对服务员说“我要一份夏威夷披萨”。披萨店（工厂）在后厨的一顿操作后，把做好的成品（对象）端给你。

### 🛠 解决什么问题？
*   **问题**: 创建逻辑复杂。例如创建一个 `Card` 对象，需要设置 ID、名字、颜色、造价、挂载3个不同的 Effect。如果在 `main` 函数里写，代码会非常臃肿。
*   **优势**: 将**对象的创建**与**对象的使用**分离。
*   **劣势**: 代码量增加。

### 🌐 现实生活类比
*   **汽车组装流水线**: 消费者只买车，不关心车是怎么在流水线上被组装起来的。

### 💻 MVP Code Template
```cpp
class PizzaFactory {
public:
    static Pizza* createPizza(string type) {
        if (type == "Hawaii") return new HawaiiPizza();
        if (type == "Pepperoni") return new PepperoniPizza();
        return nullptr;
    }
};
```

### 🔍 本项目 Case Study: `IDataLoader`
*   **代码位置**: `IDataLoader.h`, `Utils.cpp` (`MockDataLoader`)
*   **实现**:
    *   `MockDataLoader` 就是各种工厂方法的集合：`loadCards()`, `loadWonders()`.
    *   它负责所有繁琐的初始化：`makeProdCard(...)`, `w.effects.push_back(...)`。
    *   `GameController` 只需要调用 `loader->loadCards()` 就能获得准备好的全套卡牌，完全不关心这些卡牌的具体数值设置。

---

## 4. 命令模式 (Command Pattern)

### 🎓 通俗解释
**“餐厅订单”**。你在餐厅点菜，不会直接冲进厨房指挥厨师做菜。而是写一张“单子”（Command）。这张单子包含了你的桌号、菜名、忌口。服务员把单子贴在厨房，厨师空闲了就拿单子做菜。单子本身是一个对象，可以被排队、撤销、记录日志。

### 🛠 解决什么问题？
*   **问题**: 请求发送者（View/Agent）与请求接收者（Model）紧耦合。无法实现撤销、重做、宏命令。
*   **优势**: 解耦；易于扩展新的命令；支持组合命令。

### 🌐 现实生活类比
*   **遥控器**: 每个按钮都是一个 Command。可编程遥控器可以把一系列操作录制下来。

### 💻 MVP Code Template
```cpp
struct Command {
    int targetId;
    string actionType;
};
// 将命令存入队列
vector<Command> queue;
queue.push_back({1, "JUMP"});
```

### 🔍 本项目 Case Study: `Action`
*   **代码位置**: `Action.h`
*   **实现**:
    *   我们没有使用完全面向对象的 Command 类（如 `class BuildCommand : Command`），而是使用了 **Data Layout Command** (`struct Action`)。这是 C++ 游戏开发中常见的变体，利于序列化和网络传输。
    *   `Action` 结构体包含了意图 (`type`) 和参数 (`targetCardId`)。
    *   AI 可以在脑海中模拟生成无数个 `Action` 对象，测试其是否合法 (`validateAction`)，而不会对游戏产生实际副作用。

---

## 5. 组合模式 (Composite Pattern)

### 🎓 通俗解释
**“文件与文件夹”**。文件夹里可以有文件，也可以有子文件夹。如果我要“统计大小”，我可以对文件问大小，也可以对文件夹问大小（递归求和）。用户不需要区分对待“文件”和“文件夹”，把它们统称为“节点”。

### 🛠 解决什么问题？
*   **问题**: 处理部分-整体的层次结构。
*   **优势**: 客户端可以用一致的方式处理个别对象和对象组合。

### 🔍 本项目 Case Study: `CardPyramid` & `Card`
*   **代码位置**: `Model.cpp` (CardPyramid logic)
*   **实现**:
    *   **Part 1**: `Card` 本身通过 `vector<IEffect>` 组合了多个效果。对外询问 `getPoints()` 时，它递归询问内部所有 Effect 的分数总和。这体现了**一致性**。
    *   **Part 2**: `CardPyramid` 管理了一个 DAG（有向无环图）。当我们移除一张卡时，它自动处理子节点的解锁状态。虽然这不是经典的树形组合，但属于广义的结构组合模式。

---

## 6. 模型-视图-控制器 (MVC)

### 🎓 通俗解释
**“木偶戏”**。
*   **木偶 (Model)**: 只有数据和关节结构，自己不会动，不知道台下有没有观众。
*   **观众 (View)**: 只能看到木偶，负责鼓掌或喝彩。
*   **牵线人 (Controller)**: 手里拿着线，操纵木偶动，同时听观众的反应决定下一场演什么。

### 🛠 解决什么问题？
*   **问题**: 界面代码（UI）与业务逻辑（Logic）混杂，导致改界面就会弄坏逻辑，或者逻辑无法复用到新的界面（如从控制台移植到 Unity）。
*   **优势**: 关注点分离（Separation of Concerns）。

### 🔍 本项目 Case Study: 整体架构
*   **Model (`GameModel`)**: 纯粹的数据和规则。完全不知道 `std::cout` 的存在。
*   **View (`GameView`)**: 负责打印。不知道游戏规则（比如能不能打这张牌）。
*   **Controller (`GameController`)**: 接收 View/Agent 的 Input，验证后修改 Model，然后通知 View 刷新。

---

## 7. SOLID 原则教育

*   **S - SRP (单一职责)**: 
    *   **Bad**: 一个 `Player` 类既管存金币，又管计算 AI 移动，还管画自己到屏幕上。
    *   **Good (This Project)**: `Player` 只管金币/卡牌数据。`AI_Agent` 负责决策。`GameView` 负责画 `Player`。

*   **O - OCP (开闭原则)**: 
    *   **Bad**: 加新效果要修改 `Card::getPoints` 里的 switch 语句。
    *   **Good (This Project)**: 加新效果写新类，原代码不用动。

*   **L - LSP (里氏替换)**:
    *   所有 `IEffect` 子类都能无缝插入 `Card` 的 effects 列表。

*   **I - ISP (接口隔离)**:
    *   `IPlayerAgent` 只暴露决策接口，不包含渲染接口。

*   **D - DIP (依赖倒置)**:
    *   `GameController` 依赖 `IPlayerAgent` (抽象)，而不是 `RandomAgent` (具体)。
