#include "GameController.h"
#include "GameView.h"
#include "Agent.h"
#include <iostream>
#include <memory>

using namespace SevenWondersDuel;

int main() {
    // 1. 初始化
    GameView view;
    GameController game;

    // 加载数据 (请确保 gamedata.json 存在于运行目录)
    // 如果没有文件，会报错并退出
    game.initializeGame("/Users/choyichi/Desktop/7-Wonders/Code/gamedata.json");

    // 2. 游戏配置菜单
    view.renderMainMenu();

    std::unique_ptr<IPlayerAgent> agent1;
    std::unique_ptr<IPlayerAgent> agent2;

    int modeChoice;
    std::cout << "Select Game Mode:\n";
    std::cout << "  [1] Human vs Human\n";
    std::cout << "  [2] Human vs AI\n";
    std::cout << "  [3] AI vs AI (Watch Mode)\n";
    std::cout << ">> ";
    std::cin >> modeChoice;

    if (modeChoice == 1) {
        agent1 = std::make_unique<HumanAgent>();
        agent2 = std::make_unique<HumanAgent>();
    } else if (modeChoice == 3) {
        agent1 = std::make_unique<RandomAIAgent>();
        agent2 = std::make_unique<RandomAIAgent>();
    } else {
        // Default: Human vs AI
        agent1 = std::make_unique<HumanAgent>();
        agent2 = std::make_unique<RandomAIAgent>();
    }

    // 3. 开始游戏
    game.startGame();

    // 4. 主循环
    while (game.getState() != GameState::GAME_OVER) {
        const auto& model = game.getModel();

        // 渲染界面
        view.renderGame(model);

        // 确定当前行动的代理
        IPlayerAgent* currentAgent = (model.currentPlayerIndex == 0) ? agent1.get() : agent2.get();
        Player* currentPlayer = model.getCurrentPlayer();

        view.printTurnInfo(currentPlayer);

        // 如果是 AI 回合，暂停一下让玩家看清局面
        if (!currentAgent->isHuman()) {
             // 已经在 Agent 内部 sleep 了，这里不需要
        }

        // 获取决策
        bool actionSuccess = false;
        while (!actionSuccess) {
            Action action = currentAgent->decideAction(game, view);

            // 执行
            // processAction 内部会验证并返回 true/false
            // 如果是 AI 做出无效操作（理论上 RandomAI 只选 valid 的），这里会死循环？
            // 我们的 RandomAI 使用 validateAction 过滤了，所以应该是安全的。
            // 如果是 Human，输入无效后 processAction 返回 false，打印错误，循环继续。

            ActionResult val = game.validateAction(action); // 再次验证以获取错误信息用于显示

            if (game.processAction(action)) {
                actionSuccess = true;
                // 动作成功，状态已更新，进行下一次主循环
                if (!currentAgent->isHuman()) {
                     view.printMessage("AI performed an action.");
                }
            } else {
                if (currentAgent->isHuman()) {
                    view.printError("Action Failed: " + val.message);
                } else {
                    // AI 失败？这是一个 Bug，但也得防住死循环
                    view.printError("CRITICAL: AI attempted invalid action.");
                    actionSuccess = true; // 强制跳过防止死循环 (实际应抛出异常)
                }
            }
        }
    }

    // 5. 游戏结束
    view.renderGame(game.getModel()); // 最后一帧

    std::cout << "\n=========================================\n";
    std::cout << "              GAME OVER                  \n";
    std::cout << "=========================================\n";

    const auto& model = game.getModel();
    if (model.winnerIndex != -1) {
        std::cout << "WINNER: " << model.players[model.winnerIndex]->name << "!\n";
        std::string vType;
        switch(model.victoryType) {
            case VictoryType::MILITARY: vType = "Military Supremacy"; break;
            case VictoryType::SCIENCE: vType = "Scientific Supremacy"; break;
            case VictoryType::CIVILIAN: vType = "Civilian Victory (Points)"; break;
            default: vType = "Unknown";
        }
        std::cout << "Victory Type: " << vType << "\n";

        // 显示分数详情
        if (model.victoryType == VictoryType::CIVILIAN) {
             std::cout << "Final Scores:\n";
             std::cout << "  " << model.players[0]->name << ": " << model.players[0]->getScore(*model.players[1]) << "\n";
             std::cout << "  " << model.players[1]->name << ": " << model.players[1]->getScore(*model.players[0]) << "\n";
        }
    }

    return 0;
}