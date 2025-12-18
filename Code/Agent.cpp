//
// Created by choyichi on 2025/12/16.
//

#include "Agent.h"
#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <thread>

namespace SevenWondersDuel {

    Action HumanAgent::decideAction(GameController& controller, GameView& view) {
        // 直接调用 View 的提示函数
        // 注意：View 内部可能需要循环直到用户输入格式正确，
        // 但逻辑合法性（钱够不够）由 Controller 校验。
        // 为了用户体验，我们可以在这里做一个小循环：如果 View 返回无效输入，重试。

        while (true) {
            Action act = view.promptHumanAction(controller.getModel(), controller.getState());
            if (act.type == static_cast<ActionType>(-1)) {
                // 用户输入格式错误或取消，重试
                view.printError("Invalid input format. Please try again.");
                continue;
            }
            return act;
        }
    }

    Action RandomAIAgent::decideAction(GameController& controller, GameView& view) {
        // 简单的 AI 思考模拟
        std::this_thread::sleep_for(std::chrono::milliseconds(800));
        view.printMessage("AI is thinking...");

        const auto& model = controller.getModel();
        std::vector<Action> validActions;

        // 1. 遍历所有可能的动作，收集“合法”的动作
        // 对于奇迹轮抽阶段
        if (controller.getState() == GameState::WONDER_DRAFT_PHASE_1 ||
            controller.getState() == GameState::WONDER_DRAFT_PHASE_2) {

            for (auto w : model.draftPool) {
                Action act;
                act.type = ActionType::DRAFT_WONDER;
                act.targetWonderId = w->id;
                validActions.push_back(act);
            }
        }
        // 对于正常回合
        else if (controller.getState() == GameState::AGE_PLAY_PHASE) {
            auto availableSlots = model.board->cardStructure.getAvailableCards();

            for (auto slot : availableSlots) {
                if (!slot->cardPtr) continue;
                std::string cardId = slot->cardPtr->id;

                // 尝试 BUILD_CARD
                Action buildAct;
                buildAct.type = ActionType::BUILD_CARD;
                buildAct.targetCardId = cardId;
                if (controller.validateAction(buildAct).isValid) {
                    validActions.push_back(buildAct);
                }

                // 尝试 DISCARD
                Action discAct;
                discAct.type = ActionType::DISCARD_FOR_COINS;
                discAct.targetCardId = cardId;
                validActions.push_back(discAct); // 弃牌总是合法的

                // 尝试 BUILD_WONDER
                Player* me = model.getCurrentPlayer();
                for (auto w : me->unbuiltWonders) {
                    Action wonderAct;
                    wonderAct.type = ActionType::BUILD_WONDER;
                    wonderAct.targetCardId = cardId; // 垫材
                    wonderAct.targetWonderId = w->id;
                    if (controller.validateAction(wonderAct).isValid) {
                        validActions.push_back(wonderAct);
                    }
                }
            }
        }
        // 对于中断选择阶段 (如销毁卡牌)
        else if (controller.getState() == GameState::WAITING_FOR_DESTRUCTION) {
            // 随机选一个对手的棕/灰卡
            Player* opp = model.getOpponent();
            bool found = false;
            for (auto c : opp->builtCards) {
                if (c->type == CardType::RAW_MATERIAL || c->type == CardType::MANUFACTURED) {
                    Action act;
                    act.type = ActionType::SELECT_DESTRUCTION;
                    act.targetCardId = c->id;
                    validActions.push_back(act);
                    found = true;
                }
            }
            if (!found) {
                // 没有可炸的卡，必须发一个空 Action 来跳过状态
                // 暂时用空 ID 代表跳过，Controller 需处理
                 Action act;
                 act.type = ActionType::SELECT_DESTRUCTION;
                 act.targetCardId = ""; // No target
                 // 注意：validateAction 可能需要调整以允许空 target
                 // 或者直接在这里 return
                 return act;
            }
        }
        // [NEW] 弃牌堆选牌 (Mausoleum)
        else if (controller.getState() == GameState::WAITING_FOR_DISCARD_BUILD) {
             const auto& pile = model.board->discardPile;
             for (auto c : pile) {
                 Action act;
                 act.type = ActionType::SELECT_FROM_DISCARD;
                 act.targetCardId = c->id;
                 validActions.push_back(act);
             }
        }

        // 2. 随机选择一个
        if (validActions.empty()) {
            // 极罕见情况：无牌可出？应该是不可能的，至少可以弃牌。
            // 除非是在 WAITING_FOR_DESTRUCTION 且对手无牌。
            // 此时发一个空动作让 Controller 处理 Fallback
            Action noop;
            // 为了安全，默认选一个大概率不影响流程的类型，或者抛异常
            noop.type = ActionType::SELECT_DESTRUCTION; // 假装
            noop.targetCardId = "";
            return noop;
        }

        static std::mt19937 rng(std::chrono::system_clock::now().time_since_epoch().count());
        std::uniform_int_distribution<int> dist(0, validActions.size() - 1);

        Action chosen = validActions[dist(rng)];
        return chosen;
    }
}