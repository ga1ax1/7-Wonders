#include "GameController.h"
#include "GameView.h"
#include "Agent.h"
#include <iostream>
#include <memory>
#include <limits>

using namespace SevenWondersDuel;
#ifdef _WIN32
#include <windows.h>
#endif
using namespace SevenWondersDuel;

int main() {
    // Windows 控制台编码设置
#ifdef _WIN32
    // 设置控制台输出为UTF-8
    SetConsoleOutputCP(CP_UTF8);

    // 如果仍有问题，尝试以下组合：
    system("chcp 65001 > nul");  // 设置代码页为UTF-8

    // 设置控制台字体支持UTF-8
    CONSOLE_FONT_INFOEX font;
    font.cbSize = sizeof(font);
    GetCurrentConsoleFontEx(GetStdHandle(STD_OUTPUT_HANDLE), FALSE, &font);
    wcscpy(font.FaceName, L"Consolas");
    SetCurrentConsoleFontEx(GetStdHandle(STD_OUTPUT_HANDLE), FALSE, &font);
#endif
    // 1. 初始化
    GameView view;
    GameController game;

    // 加载数据 (请确保 gamedata.json 存在于运行目录)
    game.initializeGame("../gamedata.json");

    // 2. 游戏配置菜单
    view.renderMainMenu();

    std::unique_ptr<IPlayerAgent> agent1;
    std::unique_ptr<IPlayerAgent> agent2;

    int modeChoice;
    std::cin >> modeChoice;

    // IMPORTANT: Clear the buffer after reading int to prevent prompt skipping
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

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
        IPlayerAgent* currentAgent = (model.currentPlayerIndex == 0) ? agent1.get() : agent2.get();

        // 渲染逻辑优化：AI 回合主动渲染以便观看，人类回合由 promptHumanAction 内部渲染
        if (!currentAgent->isHuman()) {
            // [Updated] 传入当前 state，确保 Draft 阶段渲染正确
            view.renderGameForAI(model, game.getState());
        }

        // 获取决策
        bool actionSuccess = false;
        while (!actionSuccess) {
            // 如果是 HumanAgent，promptHumanAction 会负责清屏、渲染、报错循环
            // 如果是 RandomAI，它直接返回 Action
            Action action = currentAgent->decideAction(game, view);

            // 逻辑验证 (扣钱/规则校验)
            ActionResult val = game.validateAction(action);

            if (game.processAction(action)) {
                actionSuccess = true;
                // 成功执行后，清除错误信息 (如果有残留)
                view.clearLastError();
            } else {
                // 动作逻辑失败 (例如钱不够)
                if (currentAgent->isHuman()) {
                    // 将错误信息注入 View，并在下一次循环的 promptHumanAction 中显示
                    view.setLastError("Action Failed: " + val.message);
                } else {
                    // 对于 AI 的严重逻辑错误，直接使用标准错误流输出
                    std::cerr << "\033[1;31m[CRITICAL] AI attempted invalid action: " << val.message << "\033[0m" << std::endl;
                    actionSuccess = true; // Skip to prevent infinite loop
                }
            }
        }
    }

    // 5. 游戏结束
    view.renderGameForAI(game.getModel(), GameState::GAME_OVER); // 最后一帧

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