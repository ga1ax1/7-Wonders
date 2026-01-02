//
// Created by choyichi on 2025/12/23.
//

#include "GameStateLogic.h"
#include "GameController.h"
#include <algorithm>

namespace SevenWondersDuel {

    // Helper to access private members if needed, or use public API
    // We rely on GameController public API.

    // ==========================================================
    //  WonderDraftState
    // ==========================================================

    void WonderDraftState::onEnter(GameController& controller) {
        // Logic when entering draft state?
        // Currently handled in handleDraftWonder transitions, keeping minimal.
    }

    ActionResult WonderDraftState::validate(const Action& action, GameController& controller) {
        ActionResult result;
        result.isValid = false;

        if (action.type == ActionType::DRAFT_WONDER) {
            bool found = false;
            for(auto w : controller.getModel().getDraftPool()) {
                if(w->getId() == action.targetWonderId) found = true;
            }
            if(found) { 
                result.isValid = true; 
                return result; 
            }
            result.message = "Wonder not available in draft pool";
            return result;
        }
        result.message = "Invalid action for Draft Phase. Expected: Pick Wonder.";
        return result;
    }

    // ==========================================================
    //  AgePlayState
    // ==========================================================

    ActionResult AgePlayState::validate(const Action& action, GameController& controller) {
        ActionResult result;
        result.isValid = false;

        const GameModel& model = controller.getModel();
        const Player* currPlayer = model.getCurrentPlayer();
        const Player* opponent = model.getOpponent();

        // 1. Find Card in Pyramid
        const Card* target = model.findCardById(action.targetCardId);
        
        if (!target) { result.message = "Card not found"; return result; }

        // 2. Check Availability
        bool isAvailable = false;
        const auto& pyramid = model.getBoard()->getCardStructure();
        for(const auto& slot : pyramid) {
            if(slot.getCardPtr() && slot.getCardPtr()->getId() == target->getId()) {
                 isAvailable = true; 
                 break; // Optimization: Found it, no need to continue
            }
        }
        if (!isAvailable) { result.message = "Card is currently covered"; return result; }

        // 3. Logic by Action Type
        if (action.type == ActionType::BUILD_CARD) {
            auto costInfo = currPlayer->calculateCost(target->getCost(), *opponent, target->getType());

            // Check Chain
            if (!target->getRequiresChainTag().empty() &&
                currPlayer->getOwnedChainTags().count(target->getRequiresChainTag())) {
                costInfo.first = true; 
                costInfo.second = 0;
            }

            if (!costInfo.first) { result.message = "Insufficient resources/coins"; return result; }

            result.isValid = true;
            result.cost = costInfo.second;
            return result;
        }
        else if (action.type == ActionType::DISCARD_FOR_COINS) {
            result.isValid = true;
            return result;
        }
        else if (action.type == ActionType::BUILD_WONDER) {
            const Wonder* w = nullptr;
            for(auto ptr : currPlayer->getUnbuiltWonders()) {
                if(ptr->getId() == action.targetWonderId) { w = ptr; break; }
            }

            if (!w) { result.message = "Wonder not found in hand"; return result; }
            if (w->isBuilt()) { result.message = "Wonder already built"; return result; }

            auto costInfo = currPlayer->calculateCost(w->getCost(), *opponent, CardType::WONDER);
            if (!costInfo.first) { result.message = "Insufficient resources for Wonder"; return result; }

            result.isValid = true;
            result.cost = costInfo.second;
            return result;
        }

        result.message = "Unknown action for Age Play Phase";
        return result;
    }

    // ==========================================================
    //  TokenSelectionState
    // ==========================================================

    ActionResult TokenSelectionState::validate(const Action& action, GameController& controller) {
        ActionResult result;
        result.isValid = false;

        if (action.type == ActionType::SELECT_PROGRESS_TOKEN) {
            result.isValid = true;
            return result;
        }
        result.message = "Must select a Progress Token";
        return result;
    }

    // ==========================================================
    //  DestructionState
    // ==========================================================

    ActionResult DestructionState::validate(const Action& action, GameController& controller) {
        ActionResult result;
        result.isValid = false;

        if (action.type == ActionType::SELECT_DESTRUCTION) {
            if (action.targetCardId.empty()) {
                result.isValid = true;
                return result;
            }

            const Player* opponent = controller.getModel().getOpponent();
            bool hasCard = false;
            const Card* targetCard = nullptr;
            for (auto c : opponent->getBuiltCards()) {
                if (c->getId() == action.targetCardId) {
                    hasCard = true;
                    targetCard = c;
                    break;
                }
            }

            if (!hasCard || !targetCard) {
                result.message = "Opponent does not possess this card";
                return result;
            }

            if (targetCard->getType() != controller.getPendingDestructionType()) {
                result.message = "Invalid card color. Must destroy a specific type.";
                return result;
            }

            result.isValid = true;
            return result;
        }
        result.message = "Must select a card to destroy (or empty to skip)";
        return result;
    }

    // ==========================================================
    //  DiscardBuildState
    // ==========================================================

    ActionResult DiscardBuildState::validate(const Action& action, GameController& controller) {
        ActionResult result;
        result.isValid = false;

        if (action.type == ActionType::SELECT_FROM_DISCARD) {
            auto& pile = controller.getModel().getBoard()->getDiscardPile();
            auto it = std::find_if(pile.begin(), pile.end(), [&](Card* c){ return c->getId() == action.targetCardId; });
            if (it != pile.end()) {
                result.isValid = true;
                return result;
            }
            result.message = "Card not found in discard pile";
            return result;
        }
        result.message = "Must select a card from discard pile";
        return result;
    }

    // ==========================================================
    //  StartPlayerSelectionState
    // ==========================================================

    ActionResult StartPlayerSelectionState::validate(const Action& action, GameController& controller) {
        ActionResult result;
        result.isValid = false;

        if (action.type == ActionType::CHOOSE_STARTING_PLAYER) {
            if (action.targetCardId == "ME" || action.targetCardId == "OPPONENT") {
                result.isValid = true; return result;
            }
            result.message = "Target must be ME or OPPONENT";
            return result;
        }
        result.message = "Must choose starting player";
        return result;
    }

    void IGameStateLogic::onEnter(GameController& controller) {}

    ActionResult GameOverState::validate(const Action& action, GameController& controller) {
        return {false, 0, "Game Over"};
    }

}
