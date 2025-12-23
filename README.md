# 7 Wonders Duel (C++ Console Edition)

本项目是基于 C++17 开发的经典桌面游戏《七大奇迹：对决》的命令行版本。项目采用了多种设计模式进行重构，实现了核心逻辑、数据模型与交互界面的深度解耦，具备高度的可扩展性与可维护性。

## 1. 项目架构

项目遵循 **MVC (Model-View-Controller)** 模式的变体，并引入了状态模式与命令模式来处理复杂的棋盘游戏逻辑。

### 核心组件
- **GameModel (Model)**: 聚合根，持有玩家状态、棋盘拓扑（金字塔）、军事轨道及科技标记等原始数据。
- **GameView (View)**: 负责控制台渲染，将复杂的金字塔结构与玩家数值可视化。
- **GameController (Controller)**: 驱动游戏循环，维护当前状态机，并作为环境接口供效果系统调用。

### 设计模式应用
- **状态模式 (State Pattern)**: 通过 `IGameStateLogic` 接口处理不同阶段（如轮抽、时代进程、特殊中断）的合法性校验。
- **命令模式 (Command Pattern)**: 所有玩家行为被封装为 `IGameCommand` 对象，通过 `CommandFactory` 统一创建，确保了业务逻辑的原子性。
- **策略模式 (Strategy Pattern)**: 针对行会卡（紫色卡牌）的多样化计分规则，采用策略接口实现动态计算。
- **代理模式 (Agent Pattern)**: `IPlayerAgent` 定义了决策接口，支持人类玩家输入与随机 AI 决策的无缝切换。

## 2. 目录结构

```text
.
├── include/               # 头文件 (.h)
├── src/                   # 源文件 (.cpp)
├── data/                  # 游戏配置文件 (gamedata.json)
├── build/                 # 编译产物
├── main.cpp               # 程序入口
└── CMakeLists.txt         # 构建配置文件
```

## 3. 技术特性
- **数据驱动**: 所有的卡牌属性、奇迹效果及数值平衡均在 `data/gamedata.json` 中配置，无需修改代码即可调整游戏平衡。
- **解耦的交互系统**: `InputManager` 负责解析字符串指令并映射为 `Action` 结构，与核心逻辑通过抽象接口通信。
- **自定义 JSON 解析**: 采用轻量级 `TinyJson` 模块，减少了对第三方库的依赖。
