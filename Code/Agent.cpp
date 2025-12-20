#include "Agent.h"
#include "GameController.h"
#include "GameView.h"
#include <iostream>
#include <random>
#include <algorithm>
#include <thread>
#include <chrono>

namespace SevenWondersDuel {

    // 辅助：获取随机数引擎
    std::mt19937& getRNG() {
        static std::random_device rd;
        static std::mt19937 rng(rd());
        return rng;
    }

    // ==========================================================
    //  Human Agent
    // ==========================================================

    Action HumanAgent::decideAction(GameController& game, GameView& view) {
        return view.promptHumanAction(game.getModel(), game.getState());
    }

    // ==========================================================
    //  Random AI Agent (Robust & Validated)
    // ==========================================================

    Action RandomAIAgent::decideAction(GameController& game, GameView& view) {
        // 1. 提示 AI 正在思考
        std::cout << "\033[1;35m[AI] 正在思考...\033[0m" << std::endl;

        // 2. 模拟思考时间 (1.5秒)
        std::this_thread::sleep_for(std::chrono::milliseconds(1500));

        const GameModel& model = game.getModel();
        GameState state = game.getState();
        Action action;
        action.type = static_cast<ActionType>(-1); // Init invalid

        auto& rng = getRNG();

        // ------------------------------------------------------
        // 1. 奇迹轮抽阶段
        // ------------------------------------------------------
        if (state == GameState::WONDER_DRAFT_PHASE_1 || state == GameState::WONDER_DRAFT_PHASE_2) {
            if (!model.draftPool.empty()) {
                std::uniform_int_distribution<int> dist(0, model.draftPool.size() - 1);
                action.type = ActionType::DRAFT_WONDER;
                Wonder* selectedWonder = model.draftPool[dist(rng)];
                action.targetWonderId = selectedWonder->id;

                std::cout << "\033[1;35m[AI] 决定拿取奇迹: " << selectedWonder->name << "\033[0m\n";
                std::this_thread::sleep_for(std::chrono::milliseconds(2000)); // 决策后暂停
                return action;
            }
        }

        // ------------------------------------------------------
        // 2. 特殊交互状态 (Interrupts)
        // ------------------------------------------------------

        // A. 科技标记 (配对)
        else if (state == GameState::WAITING_FOR_TOKEN_SELECTION_PAIR) {
            const auto& tokens = model.board->availableProgressTokens;
            if (!tokens.empty()) {
                std::uniform_int_distribution<int> dist(0, tokens.size() - 1);
                action.type = ActionType::SELECT_PROGRESS_TOKEN;
                action.selectedToken = tokens[dist(rng)];

                std::cout << "\033[1;35m[AI] 获得科技配对奖励，选择标记...\033[0m\n";
                std::this_thread::sleep_for(std::chrono::milliseconds(2000)); // 决策后暂停
                return action;
            }
        }

        // B. 科技标记 (图书馆盒子)
        else if (state == GameState::WAITING_FOR_TOKEN_SELECTION_LIB) {
            const auto& tokens = model.board->boxProgressTokens;
            if (!tokens.empty()) {
                std::uniform_int_distribution<int> dist(0, tokens.size() - 1);
                action.type = ActionType::SELECT_PROGRESS_TOKEN;
                action.selectedToken = tokens[dist(rng)];

                std::cout << "\033[1;35m[AI] 触发图书馆效果，从盒子中选择标记...\033[0m\n";
                std::this_thread::sleep_for(std::chrono::milliseconds(2000)); // 决策后暂停
                return action;
            }
        }

        // C. 摧毁对手卡牌
        else if (state == GameState::WAITING_FOR_DESTRUCTION) {
            const Player* opp = model.getOpponent();
            std::vector<Card*> candidates = opp->builtCards;
            std::shuffle(candidates.begin(), candidates.end(), rng);

            // 1. 尝试摧毁
            for (auto c : candidates) {
                Action tryDestruct;
                tryDestruct.type = ActionType::SELECT_DESTRUCTION;
                tryDestruct.targetCardId = c->id;

                if (game.validateAction(tryDestruct).isValid) {
                    std::cout << "\033[1;35m[AI] 决定摧毁对手的卡牌: " << c->name << "\033[0m\n";
                    std::this_thread::sleep_for(std::chrono::milliseconds(2000)); // 决策后暂停
                    return tryDestruct;
                }
            }

            // 2. 放弃
            Action skipAction;
            skipAction.type = ActionType::SELECT_DESTRUCTION;
            skipAction.targetCardId = "";
            if (game.validateAction(skipAction).isValid) {
                std::cout << "\033[1;35m[AI] 没有合适的目标，选择跳过摧毁。\033[0m\n";
                std::this_thread::sleep_for(std::chrono::milliseconds(2000)); // 决策后暂停
                return skipAction;
            }

            return action;
        }

        // D. 陵墓复活
        else if (state == GameState::WAITING_FOR_DISCARD_BUILD) {
            const auto& pile = model.board->discardPile;
            if (!pile.empty()) {
                std::vector<Card*> candidates = pile;
                std::shuffle(candidates.begin(), candidates.end(), rng);

                for (auto c : candidates) {
                    Action tryResurrect;
                    tryResurrect.type = ActionType::SELECT_FROM_DISCARD;
                    tryResurrect.targetCardId = c->id;
                    if (game.validateAction(tryResurrect).isValid) {
                        std::cout << "\033[1;35m[AI] 决定从弃牌堆复活: " << c->name << "\033[0m\n";
                        std::this_thread::sleep_for(std::chrono::milliseconds(2000)); // 决策后暂停
                        return tryResurrect;
                    }
                }
            }
            return action;
        }

        // E. 选择先手
        else if (state == GameState::WAITING_FOR_START_PLAYER_SELECTION) {
            std::uniform_int_distribution<int> dist(0, 1);
            action.type = ActionType::CHOOSE_STARTING_PLAYER;
            bool chooseMe = (dist(rng) == 0);
            action.targetCardId = chooseMe ? "ME" : "OPPONENT";

            std::cout << "\033[1;35m[AI] 决定下个时代 " << (chooseMe ? "自己" : "对手") << " 先手。\033[0m\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(2000)); // 决策后暂停
            return action;
        }

        // ------------------------------------------------------
        // 3. 主游戏阶段 (Age Play)
        // ------------------------------------------------------
        else if (state == GameState::AGE_PLAY_PHASE) {
            const Player* me = model.getCurrentPlayer();

            // 获取所有可用的金字塔卡牌
            std::vector<const CardSlot*> availableSlots = model.board->cardStructure.getAvailableCards();
            std::vector<const CardSlot*> validSlots;
            for(auto s : availableSlots) {
                if(s->cardPtr && s->isFaceUp) validSlots.push_back(s);
            }

            if (validSlots.empty()) return action;

            std::shuffle(validSlots.begin(), validSlots.end(), rng);

            // --- 策略 A: 尝试建造奇迹 (20% 概率) ---
            std::uniform_int_distribution<int> dice(1, 100);
            if (dice(rng) <= 20 && !me->unbuiltWonders.empty()) {
                for (auto w : me->unbuiltWonders) {
                    for(auto slot : validSlots) {
                        Action tryWonder;
                        tryWonder.type = ActionType::BUILD_WONDER;
                        tryWonder.targetCardId = slot->cardPtr->id;
                        tryWonder.targetWonderId = w->id;

                        if (game.validateAction(tryWonder).isValid) {
                            std::cout << "\033[1;35m[AI] 决定建造奇迹: " << w->name << " (使用卡牌: " << slot->cardPtr->name << ")\033[0m\n";
                            std::this_thread::sleep_for(std::chrono::milliseconds(2000)); // 决策后暂停
                            return tryWonder;
                        }
                    }
                }
            }

            // --- 策略 B: 尝试建造卡牌 ---
            for (auto slot : validSlots) {
                Action tryBuild;
                tryBuild.type = ActionType::BUILD_CARD;
                tryBuild.targetCardId = slot->cardPtr->id;

                if (game.validateAction(tryBuild).isValid) {
                    std::cout << "\033[1;35m[AI] 决定建造卡牌: " << slot->cardPtr->name << "\033[0m\n";
                    std::this_thread::sleep_for(std::chrono::milliseconds(2000)); // 决策后暂停
                    return tryBuild;
                }
            }

            // --- 策略 C: 弃牌换钱 (Fallback) ---
            action.type = ActionType::DISCARD_FOR_COINS;
            action.targetCardId = validSlots[0]->cardPtr->id;
            std::cout << "\033[1;35m[AI] 资源不足，决定弃掉卡牌换钱: " << validSlots[0]->cardPtr->name << "\033[0m\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(2000)); // 决策后暂停
            return action;
        }

        return action;
    }

}