# Seven Wonders Duel - Complete Educational Documentation

This document consolidates the Project README, Design Analysis, and Design Patterns Guide into a single resource.

---

# Part 1: Project README & Architecture Overview

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


---
---

# Part 2: Architecture & Design Analysis (SOLID Principles)

# Architecture & Design Analysis

本文档详细分析了《七大奇迹对决》C++ 实现中的架构设计，重点说明 **SOLID 原则** 的体现以及 **设计模式** 的应用与取舍。

---

## 1. SOLID 原则分析

### 1.1 单一职责原则 (Single Responsibility Principle - SRP)
> **原则**: 一个类应该只有一个引起它变化的原因。

*   **体现**:
    *   **`GameModel`**: 只负责维护数据状态（Data Holding）和核心规则计算（Victory Check），不包含任何 UI 输出或输入逻辑。
    *   **`GameView`**: 只负责渲染（`std::cout`），不包含游戏逻辑。
    *   **`GameController`**: 只负责流程控制（Turn loop）、状态流转和动作分发。它不直接操作数据细节，而是协调 Model 和 View。
    *   **`IEffect`**: 每个具体的 Effect 类（如 `MilitaryEffect`, `ProductionEffect`）只负责实现单一的游戏机制逻辑。

### 1.2 开闭原则 (Open/Closed Principle - OCP)
> **原则**: 软件实体（类、模块、函数等）应该对扩展开放，对修改关闭。

*   **体现**:
    *   **卡牌效果系统**: 如果需要增加一种新的卡牌效果（比如“模仿对手的科技”），我们只需要创建一个新的类 `CopyScienceEffect` 继承自 `IEffect`，并在 `IDataLoader` 中挂载即可。**无需修改** `Card` 类、`Player` 类或 `GameController` 的核心逻辑。
    *   **AI 扩展**: 通过 `IPlayerAgent` 接口，我们可以添加新的 AI 策略（比如 `MCTS_Agent`），而不需要修改 `GameController` 的代码。

### 1.3 里氏替换原则 (Liskov Substitution Principle - LSP)
> **原则**: 子类对象必须能够替换掉所有父类对象。

*   **体现**:
    *   **`IEffect` 体系**: 所有使用 `IEffect*` 的地方（如 `Card::onBuild`），都可以安全地使用 `MilitaryEffect` 或 `ScienceEffect`，因为它们严格遵守了 `apply()` 接口的契约，没有破坏性的副作用（如抛出未捕获异常或修改不该修改的状态）。
    *   **`IPlayerAgent` 体系**: `HumanAgent` 和 `AI_RandomAgent` 都可以被 `GameController` 无差别调用，行为一致（接收 Model，返回 Action）。

### 1.4 接口隔离原则 (Interface Segregation Principle - ISP)
> **原则**: 不应强迫客户端依赖于它们不用的接口。

*   **体现**:
    *   **`IPlayerAgent`**: 接口中仅包含决策所需的函数 (`decideNextMove`, `selectXXX`)。没有掺杂 `render()` 或 `updateModel()` 等 AI 不需要关心的方法。
    *   **`IDataLoader`**: 专注于数据加载，与游戏运行时的逻辑完全隔离。

### 1.5 依赖倒置原则 (Dependency Inversion Principle - DIP)
> **原则**: 高层模块不应依赖于低层模块，二者都应依赖于抽象。

*   **体现**:
    *   **Controller 依赖抽象**: `GameController` 不依赖具体的 `HumanAgent` 或 `AICalculator`，而是依赖于抽象接口 `IPlayerAgent`。这使得我们可以在 `main.cpp` 中随意注入 Agent 类型，而不用改动 Controller 代码。
    *   **卡牌依赖抽象**: `Card` 类不依赖具体的 `ProductionEffect` 类，而是持有 `IEffect` 的指针。

---

## 2. 设计模式应用分析 (Design Patterns)

根据请求，我们将对照常见设计模式列表，逐一分析本项目的使用情况。

### 2.1 创建型模式 (Creational Patterns)

#### ✅ Abstract Factory / Factory Method (抽象工厂/工厂方法)
*   **使用**: **`IDataLoader` (及 `MockDataLoader`)**
*   **原因**: 游戏数据的来源可能是多样的（硬编码 Mock、JSON 文件、数据库）。`IDataLoader` 定义了创建 `Card` 和 `Wonder` 对象的接口。`MockDataLoader` 是具体工厂，负责生产具体的对象实例。这隔离了数据的**创建**与**使用**。

#### ✅ Singleton (单例模式)
*   **使用情况**: **明确未使用 (AVOIDED)**
*   **原因**: 本代码中没有使用全局单例（如 `GameService::getInstance()`）。所有的依赖（Model, View, Loader）都通过构造函数注入到 `GameController` 中。
*   **理由**: 单例模式会隐藏依赖关系，导致测试困难和代码耦合。依赖注入是更现代、更清洁的替代方案。

#### ✅ Builder (建造者模式)
*   **使用**: **隐式使用 (在 `MockDataLoader` 中)**
*   **原因**: 在 `MockDataLoader` 内部，我们分步构建一个复杂的 `Card` 对象（先设ID，再设费用，再挂载 Effect A，再挂载 Effect B）。虽然没有封装成标准的 `CardBuilder` 类，但构建逻辑体现了 Builder 的思想：逐步组装复杂对象。

#### ― Prototype (原型模式)
*   **未使用**。
*   **原因**: 游戏中的卡牌通常是唯一的（或由工厂全新创建）。我们不需要通过“克隆”一个现有对象来创建新对象。

---

### 2.2 结构型模式 (Structural Patterns)

#### ✅ Strategy (策略模式)
*   **使用**: **核心模式 - `IEffect`** 和 **`IPlayerAgent`**
*   **原因**: 这是本项目最重要的模式。
    *   **`IEffect`**: 将卡牌的“行为”抽象为策略。每张卡牌可以动态组合不同的策略（产资源策略、军事策略）。这比继承（`MilitaryCard`）灵活得多。
    *   **`IPlayerAgent`**: 使得“决策算法”可以热替换。人类输入、随机AI、极小化极大算法都是不同的策略。

#### ✅ Composite (组合模式)
*   **使用**: **`Card` 类** 和 **`CardPyramid`**
*   **原因**:
    *   **`Card`**: 拥有一组 `vector<IEffect>`。调用 `card.onBuild` 时，它会遍历执行所有 Effect。这是将“一对多”的关系处理为“一对一”接口的典型组合模式体现。
    *   **`CardPyramid`**: 虽然实现为图结构，但它管理着部分整体的层次结构，体现了组合关系。

#### ✅ Facade (外观模式)
*   **使用**: **`GameController`**
*   **原因**: 对于 `main` 函数（客户端）来说，`GameController` 封装了游戏启动、循环、渲染、交互的所有复杂性。外部只需要调用 `initializeGame` 和 `runGameLoop` 两个简单接口。

#### ― Adapter (适配器模式)
*   **未使用**。
*   **原因**: 我们从头设计了接口，不需要适配旧的、不兼容的第三方库或接口。

#### ― Decorator (装饰器模式)
*   **未使用**。
*   **原因**: 卡牌的效果确定后通常不变。如果游戏中有“给某张已有卡牌临时增加一个效果”的机制，就会用到装饰器。目前需求中卡牌状态较固定。

#### ― Flyweight (享元模式)
*   **潜在使用**: `std::shared_ptr<IEffect>`
*   **原因**: 目前的代码结构允许不同的卡牌共享同一个 `IEffect` 实例（例如多张卡都产出 `WOOD`，可以指向同一个 `ProductionEffect` 对象）。虽然 `MockDataLoader` 中每次都 `make_shared`，但架构上支持享元以节省内存。

---

### 2.3 行为型模式 (Behavioral Patterns)

#### ✅ Command (命令模式)
*   **使用**: **`Action` 结构体**
*   **原因**: 玩家的操作（建牌、弃牌、建奇迹）被封装为一个纯数据对象 `Action`。
    *   这使得我们可以轻松地将操作放入队列。
    *   AI 可以生成多个 `Action` 对象进行评估。
    *   Controller 可以统一验证 (`validateAction`) 和执行 (`processAction`) 这些命令对象。

#### ✅ State (状态模式)
*   **使用**: **`GameState` 枚举与 `GameController` 状态机**
*   **原因**: 游戏流程非常复杂（轮抽 -> 出牌 -> 选Token中断 -> 毁灭中断 -> 结算）。使用明确的状态机管理，使得每个阶段的处理逻辑清晰分离（`handleWonderDraft` vs `handlePlayPhase` vs `handleDestruction`）。如果不主要使用状态模式，代码将充斥着大量的 `if(flag)` 语句。

#### ✅ Observer (观察者模式)
*   **使用**: **`GameView` (作为观察者)**
*   **原因**: 虽然没有实现标准的 `Subscription` 机制，但 `GameController` 在状态更新后主动调用 `view->renderGameState()`，体现了推模式（Push Model）的通知机制。View 被动接收数据变化并更新。

#### ― Mediator (中介者模式)
*   **使用**: **`GameController`**
*   **原因**: `GameController` 就是典型的中介者。`HumanAgent`（View的一部分）不直接修改 `Model`，而是发送指令给 Controller；`Model` 也不直接通知 View，而是通过 Controller 传递。它解耦了各个组件的多对多交互。

#### ― Template Method (模板方法)
*   **未使用**。
*   **原因**: `IEffect` 使用的是接口（多态），而不是具有默认实现的抽象基类。如果我们在基类中定义了 `execute()` 流程（如 `check -> apply -> log`），而子类只实现 `apply`，那就是模板方法。目前直接使用了虚函数。

#### ― Visitor (访问者模式)
*   **未使用**。
*   **原因**: 我们的 `IEffect` 只有 `apply` 和 `calculateScore` 两个固定操作，且操作逻辑相对内聚。如果我们需要给卡牌增加一系列无关的新操作（如“导出为XML”、“统计平衡性”），且不想修改类结构时，会考虑 Visitor。目前尚未遇到此需求。

---


---
---

# Part 3: Design Patterns Educational Guide

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
