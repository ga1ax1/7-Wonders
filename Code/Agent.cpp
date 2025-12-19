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
        // 模拟思考时间
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

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
                action.targetWonderId = model.draftPool[dist(rng)]->id;
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
                return action;
            }
        }

        // C. 摧毁对手卡牌 (关键修复：循环尝试)
        else if (state == GameState::WAITING_FOR_DESTRUCTION) {
            const Player* opp = model.getOpponent();

            // 获取对手所有卡牌，打乱顺序尝试
            std::vector<Card*> candidates = opp->builtCards;
            std::shuffle(candidates.begin(), candidates.end(), rng);

            // 1. 尝试摧毁其中一张
            for (auto c : candidates) {
                Action tryDestruct;
                tryDestruct.type = ActionType::SELECT_DESTRUCTION;
                tryDestruct.targetCardId = c->id;

                // 利用游戏逻辑验证是否符合颜色要求
                if (game.validateAction(tryDestruct).isValid) {
                    return tryDestruct;
                }
            }

            // 2. 如果都不行 (或没有目标)，尝试 SKIP (空ID)
            Action skipAction;
            skipAction.type = ActionType::SELECT_DESTRUCTION;
            skipAction.targetCardId = "";
            if (game.validateAction(skipAction).isValid) {
                return skipAction;
            }

            return action; // Should not reach here
        }

        // D. 陵墓复活
        else if (state == GameState::WAITING_FOR_DISCARD_BUILD) {
            const auto& pile = model.board->discardPile;
            if (!pile.empty()) {
                // 同样打乱尝试，防止死循环（虽然弃牌堆通常没有限制）
                std::vector<Card*> candidates = pile;
                std::shuffle(candidates.begin(), candidates.end(), rng);

                for (auto c : candidates) {
                    Action tryResurrect;
                    tryResurrect.type = ActionType::SELECT_FROM_DISCARD;
                    tryResurrect.targetCardId = c->id;
                    if (game.validateAction(tryResurrect).isValid) {
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
            action.targetCardId = (dist(rng) == 0) ? "ME" : "OPPONENT";
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
                // 确保有牌且朝上 (处理 Age 2 底部/顶部可用问题：只要逻辑认为是 FaceUp，AI 就可以选)
                if(s->cardPtr && s->isFaceUp) validSlots.push_back(s);
            }

            if (validSlots.empty()) return action;

            // 打乱顺序
            std::shuffle(validSlots.begin(), validSlots.end(), rng);

            // --- 策略 A: 尝试建造奇迹 (20% 概率) ---
            std::uniform_int_distribution<int> dice(1, 100);
            if (dice(rng) <= 20 && !me->unbuiltWonders.empty()) {
                for (auto w : me->unbuiltWonders) {
                    // 尝试用任意一张可用卡作为垫材
                    for(auto slot : validSlots) {
                        Action tryWonder;
                        tryWonder.type = ActionType::BUILD_WONDER;
                        tryWonder.targetCardId = slot->cardPtr->id;
                        tryWonder.targetWonderId = w->id;

                        // 使用 validateAction 检查钱和资源
                        if (game.validateAction(tryWonder).isValid) {
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

                // 使用 validateAction 检查所有条件
                if (game.validateAction(tryBuild).isValid) {
                    return tryBuild;
                }
            }

            // --- 策略 C: 弃牌换钱 (Fallback) ---
            // 只要是 validSlot 里的牌，弃牌总是合法的
            action.type = ActionType::DISCARD_FOR_COINS;
            action.targetCardId = validSlots[0]->cardPtr->id;
            return action;
        }

        return action;
    }

}