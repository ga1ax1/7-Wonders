//
// Created by choyichi on 2025/12/15.
//

#ifndef SEVEN_WONDERS_DUEL_GAMECONTROLLER_H
#define SEVEN_WONDERS_DUEL_GAMECONTROLLER_H

#include "Global.h"
#include "Player.h"
#include "Board.h"
#include "Card.h"
#include <memory>
#include <vector>
#include <string>
#include <random>

namespace SevenWondersDuel {

    // 动作参数结构体
    struct Action {
        ActionType type;
        std::string targetCardId;   // 选中的金字塔卡牌 / 弃牌堆卡牌 / 对手卡牌
        std::string targetWonderId; // 选中的手牌奇迹 (用于建造奇迹时)
        ProgressToken selectedToken = ProgressToken::NONE;
        ResourceType chosenResource = ResourceType::WOOD; // 用于极其罕见的多选一判定
    };

    // 动作结果反馈
    struct ActionResult {
        bool isValid;
        int cost = 0;
        std::string message;
    };

    // 模型聚合根 (Model Layer Root)
    struct GameModel {
        std::vector<std::unique_ptr<Player>> players;
        std::unique_ptr<Board> board;

        int currentAge = 0;
        int currentPlayerIndex = 0; // 0 或 1
        int winnerIndex = -1;       // -1:未结束, 0:P1胜, 1:P2胜
        VictoryType victoryType = VictoryType::NONE;

        // 奇迹轮抽池
        std::vector<Wonder*> draftPool;        // 当前轮抽区可见的4张
        std::vector<Wonder*> remainingWonders; // 剩下的奇迹（保证不重复）

        // 数据池 (从JSON加载的所有原始数据)
        std::vector<Card> allCards;
        std::vector<Wonder> allWonders;

        // 完整日志
        std::vector<std::string> gameLog;

        GameModel() {
            board = std::make_unique<Board>();
        }

        Player* getCurrentPlayer() const { return players[currentPlayerIndex].get(); }
        Player* getOpponent() const { return players[1 - currentPlayerIndex].get(); }

        void addLog(const std::string& msg) {
            gameLog.push_back(msg);
        }

        // 获取当前金字塔剩余卡牌数
        int getRemainingCardCount() const {
            int count = 0;
            for(const auto& slot : board->cardStructure.slots) {
                if (!slot.isRemoved) count++;
            }
            return count;
        }
    };

    // 游戏控制器
    class GameController {
    public:
        GameController();
        ~GameController() = default;

        // --- 初始化与流程 ---

        // 加载数据并重置游戏 (需传入JSON路径)
        void initializeGame(const std::string& jsonPath);

        // 开始游戏 (进入奇迹轮抽)
        void startGame();

        // --- 核心交互接口 ---

        // 获取当前状态
        GameState getState() const { return currentState; }

        // 获取只读的模型引用 (供UI渲染)
        const GameModel& getModel() const { return *model; }

        // 验证动作是否合法
        ActionResult validateAction(const Action& action);

        // 执行动作
        // 返回 true 表示动作成功执行并触发了状态变更
        bool processAction(const Action& action);

        // --- 辅助逻辑 (公开给 Effects 使用) ---

        void setState(GameState newState) { currentState = newState; }
        Board* getBoard() { return model->board.get(); }

        // 触发再来一回合
        void grantExtraTurn() { extraTurnPending = true; }

    private:
        std::unique_ptr<GameModel> model;
        GameState currentState = GameState::WONDER_DRAFT_PHASE_1;

        // 内部状态标记
        bool extraTurnPending = false;
        std::mt19937 rng; // 随机数生成器

        // --- 内部流程方法 ---

        void loadData(const std::string& path);

        // 时代设置流程
        void setupAge(int age);
        std::vector<Card*> prepareDeckForAge(int age);

        // 修正后的奇迹准备逻辑
        void initWondersDeck();
        void dealWondersToDraft();

        // 切换玩家
        void switchPlayer();

        // 胜利检测
        void checkVictoryConditions();
        void resolveMilitaryLoot(const std::vector<int>& lootEvents);

        // 具体的动作处理器
        void handleDraftWonder(const Action& action);
        void handleBuildCard(const Action& action);
        void handleDiscardCard(const Action& action);
        void handleBuildWonder(const Action& action);
        void handleSelectProgressToken(const Action& action);
        void handleDestruction(const Action& action);

        // 辅助：从ID查找对象
        Card* findCardInPyramid(const std::string& id);
        Wonder* findWonderInHand(Player* p, const std::string& id);
    };
}

#endif // SEVEN_WONDERS_DUEL_GAMECONTROLLER_H