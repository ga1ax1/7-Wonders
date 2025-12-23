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

## 3. 总结

该架构设计重点为了解决以下问题：
1.  **卡牌效果的多样性** -> 使用 **Strategy (IEffect)**。
2.  **游戏流程的复杂性** -> 使用 **State Machine (in Controller)**。
3.  **不同智能体的接入** -> 使用 **Strategy (Agent)**。
4.  **数据构建的灵活性** -> 使用 **Factory (DataLoader)**。

通过严格遵循 SOLID 原则，代码具有极高的**可测试性**（可以直接测试 Model 逻辑而无关 View）和**可维护性**（新增卡牌或 AI 无需重构核心）。
