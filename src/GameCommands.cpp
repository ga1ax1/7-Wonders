//
// Created by choyichi on 2025/12/23.
//

#include "GameCommands.h"
#include "GameController.h"
#include <algorithm>
#include <iostream>

namespace SevenWondersDuel {

    std::unique_ptr<IGameCommand> CommandFactory::createCommand(const Action& action) {
        switch (action.type) {
            case ActionType::DRAFT_WONDER: return std::make_unique<DraftWonderCommand>(action.targetWonderId);
            case ActionType::BUILD_CARD: return std::make_unique<BuildCardCommand>(action.targetCardId);
            case ActionType::DISCARD_FOR_COINS: return std::make_unique<DiscardCardCommand>(action.targetCardId);
            case ActionType::BUILD_WONDER: return std::make_unique<BuildWonderCommand>(action.targetCardId, action.targetWonderId);
            case ActionType::SELECT_PROGRESS_TOKEN: return std::make_unique<SelectProgressTokenCommand>(action.selectedToken);
            case ActionType::SELECT_DESTRUCTION: return std::make_unique<DestructionCommand>(action.targetCardId);
            case ActionType::SELECT_FROM_DISCARD: return std::make_unique<SelectFromDiscardCommand>(action.targetCardId);
            case ActionType::CHOOSE_STARTING_PLAYER: return std::make_unique<ChooseStartingPlayerCommand>(action.targetCardId);
            default: return nullptr;
        }
    }

    // ==========================================================
    //  DraftWonderCommand
    // ==========================================================

    DraftWonderCommand::DraftWonderCommand(std::string id) : wonderId(std::move(id)) {}

    void DraftWonderCommand::execute(GameController& controller) {
        auto& model = *controller.m_model;
        Player* currPlayer = model.getCurrentPlayerMut();

        auto& pool = model.getDraftPool();
        auto it = std::find_if(pool.begin(), pool.end(), [&](Wonder* w){ return w->getId() == wonderId; });

        if (it != pool.end()) {
            Wonder* w = *it;
            currPlayer->addUnbuiltWonder(w);
            model.removeFromDraftPool(w->getId());

            model.addLog("[" + currPlayer->getName() + "] drafted wonder: " + w->getName());

            bool shouldSwitch = true;
            if (controller.m_draftTurnCount == 1) shouldSwitch = false;

            controller.m_draftTurnCount++;

            if (model.getDraftPool().empty()) {
                if (controller.m_currentState == GameState::WONDER_DRAFT_PHASE_1) {
                    controller.setState(GameState::WONDER_DRAFT_PHASE_2);
                    controller.dealWondersToDraft();
                    controller.m_draftTurnCount = 0;
                    model.setCurrentPlayerIndex(1);
                    model.addLog("[System] Wonder Draft Phase 2 Begins. Player 2 starts.");
                } else {
                    controller.setupAge(1);
                    model.setCurrentPlayerIndex(0);
                }
            } else {
                if (shouldSwitch) {
                    model.setCurrentPlayerIndex(1 - model.getCurrentPlayerIndex());
                }
            }
        }
    }

    // ==========================================================
    //  BuildCardCommand
    // ==========================================================

    BuildCardCommand::BuildCardCommand(std::string id) : cardId(std::move(id)) {}

    void BuildCardCommand::execute(GameController& controller) {
        auto& model = *controller.m_model;
        Player* currPlayer = model.getCurrentPlayerMut();
        Player* opponent = model.getOpponentMut();
        Card* targetCard = controller.findCardInPyramid(cardId);

        auto costInfo = currPlayer->calculateCost(targetCard->getCost(), *opponent, targetCard->getType());
        
        bool isChain = false;
        if (!targetCard->getRequiresChainTag().empty() &&
            currPlayer->getOwnedChainTags().count(targetCard->getRequiresChainTag())) {
            costInfo.second = 0;
            isChain = true;
        }

        currPlayer->payCoins(costInfo.second);

        model.getBoardMut()->removeCardFromPyramid(targetCard->getId());
        currPlayer->constructCard(targetCard);

        model.addLog("[" + currPlayer->getName() + "] built " + targetCard->getName());

        if (isChain && currPlayer->getProgressTokens().count(ProgressToken::URBANISM)) {
            currPlayer->gainCoins(Config::URBANISM_CHAIN_BONUS);
            model.addLog("[Effect] Urbanism: +4 coins from chain build.");
        }

        for(auto& eff : targetCard->getEffects()) {
            eff->apply(currPlayer, opponent, &controller, &controller);
        }

        if (controller.checkForNewSciencePairs(currPlayer)) {
            return;
        }

        if (controller.m_currentState == GameState::AGE_PLAY_PHASE) {
             controller.onTurnEnd();
        }
    }

    // ==========================================================
    //  DiscardCardCommand
    // ==========================================================

    DiscardCardCommand::DiscardCardCommand(std::string id) : cardId(std::move(id)) {}

    void DiscardCardCommand::execute(GameController& controller) {
        auto& model = *controller.m_model;
        Player* currPlayer = model.getCurrentPlayerMut();
        Card* targetCard = controller.findCardInPyramid(cardId);

        model.getBoardMut()->removeCardFromPyramid(targetCard->getId());
        model.getBoardMut()->addToDiscardPile(targetCard);

        int gain = Config::BASE_DISCARD_GAIN + currPlayer->getCardCount(CardType::COMMERCIAL);
        currPlayer->gainCoins(gain);

        model.addLog("[" + currPlayer->getName() + "] discarded " + targetCard->getName() + " (+ " + std::to_string(gain) + " coins)");

        controller.onTurnEnd();
    }

    // ==========================================================
    //  BuildWonderCommand
    // ==========================================================

    BuildWonderCommand::BuildWonderCommand(std::string cid, std::string wid) : cardId(std::move(cid)), wonderId(std::move(wid)) {}

    void BuildWonderCommand::execute(GameController& controller) {
        auto& model = *controller.m_model;
        Player* currPlayer = model.getCurrentPlayerMut();
        Player* opponent = model.getOpponentMut();
        Card* pyramidCard = controller.findCardInPyramid(cardId);
        Wonder* wonder = controller.findWonderInHand(currPlayer, wonderId);

        auto costInfo = currPlayer->calculateCost(wonder->getCost(), *opponent, CardType::WONDER);
        currPlayer->payCoins(costInfo.second);

        model.getBoardMut()->removeCardFromPyramid(pyramidCard->getId());
        currPlayer->constructWonder(wonder->getId(), pyramidCard);

        model.addLog("[" + currPlayer->getName() + "] built WONDER: " + wonder->getName() + "!");

        for(auto& eff : wonder->getEffects()) {
            eff->apply(currPlayer, opponent, &controller, &controller);
        }

        int totalBuilt = model.getPlayers()[0]->getBuiltWonders().size() + model.getPlayers()[1]->getBuiltWonders().size();
        if (totalBuilt == Config::MAX_TOTAL_WONDERS) {
            model.addLog("[System] 7 Wonders built! The 8th wonder is removed.");
            model.getPlayers()[0]->clearUnbuiltWonders();
            model.getPlayers()[1]->clearUnbuiltWonders();
        }

        if (currPlayer->getProgressTokens().count(ProgressToken::THEOLOGY)) {
             controller.grantExtraTurn();
             model.addLog("[Effect] Theology Token grants an Extra Turn!");
        }

        if (controller.checkForNewSciencePairs(currPlayer)) {
            return;
        }

        if (controller.m_currentState == GameState::AGE_PLAY_PHASE) {
            controller.onTurnEnd();
        }
    }

    // ==========================================================
    //  SelectProgressTokenCommand
    // ==========================================================

    SelectProgressTokenCommand::SelectProgressTokenCommand(ProgressToken t) : token(t) {}

    void SelectProgressTokenCommand::execute(GameController& controller) {
        auto& model = *controller.m_model;
        Player* currPlayer = model.getCurrentPlayerMut();

        bool success = false;
        if (controller.m_currentState == GameState::WAITING_FOR_TOKEN_SELECTION_PAIR) {
            success = model.getBoardMut()->removeAvailableProgressToken(token);
        } else if (controller.m_currentState == GameState::WAITING_FOR_TOKEN_SELECTION_LIB) {
            success = model.getBoardMut()->removeBoxProgressToken(token);
        }

        if (success) {
            currPlayer->addProgressToken(token);
            model.addLog("[" + currPlayer->getName() + "] selected a Progress Token.");

            if (token == ProgressToken::URBANISM) {
                currPlayer->gainCoins(Config::URBANISM_TOKEN_BONUS);
                model.addLog("[Effect] Urbanism: +6 coins immediately.");
            }

            controller.setState(GameState::AGE_PLAY_PHASE);

            if (token == ProgressToken::LAW) {
                if (controller.checkForNewSciencePairs(currPlayer)) {
                    return;
                }
            }

            controller.onTurnEnd();
        }
    }

    // ==========================================================
    //  DestructionCommand
    // ==========================================================

    DestructionCommand::DestructionCommand(std::string tid) : targetId(std::move(tid)) {}

    void DestructionCommand::execute(GameController& controller) {
        auto& model = *controller.m_model;
        if (targetId.empty()) {
            model.addLog("[System] Destruction skipped.");
            controller.setState(GameState::AGE_PLAY_PHASE);
            controller.onTurnEnd();
            return;
        }

        Player* opponent = model.getOpponentMut();
        Card* target = nullptr;
        for(auto c : opponent->getBuiltCards()) {
            if (c->getId() == targetId) {
                target = c; break;
            }
        }

        if (target) {
             model.getBoardMut()->destroyCard(opponent, target->getType());
             model.addLog("[System] " + opponent->getName() + "'s card " + target->getName() + " destroyed.");
        }

        controller.setState(GameState::AGE_PLAY_PHASE);
        controller.onTurnEnd();
    }

    // ==========================================================
    //  SelectFromDiscardCommand
    // ==========================================================

    SelectFromDiscardCommand::SelectFromDiscardCommand(std::string id) : cardId(std::move(id)) {}

    void SelectFromDiscardCommand::execute(GameController& controller) {
        auto& model = *controller.m_model;
        Player* currPlayer = model.getCurrentPlayerMut();
        Player* opponent = model.getOpponentMut();

        Card* card = model.getBoardMut()->removeCardFromDiscardPile(cardId);

        if (card) {
            currPlayer->constructCard(card);

            model.addLog("[" + currPlayer->getName() + "] resurrected " + card->getName() + " from discard!");

            for(auto& eff : card->getEffects()) {
                eff->apply(currPlayer, opponent, &controller, &controller);
            }

             if (controller.checkForNewSciencePairs(currPlayer)) {
                return;
            }
        }

        controller.setState(GameState::AGE_PLAY_PHASE);
        controller.onTurnEnd();
    }

    // ==========================================================
    //  ChooseStartingPlayerCommand
    // ==========================================================

    ChooseStartingPlayerCommand::ChooseStartingPlayerCommand(std::string tid) : targetId(std::move(tid)) {}

    void ChooseStartingPlayerCommand::execute(GameController& controller) {
        auto& model = *controller.m_model;
        Player* curr = model.getCurrentPlayerMut();

        int nextStarter = -1;
        if (targetId == "ME") {
            nextStarter = model.getCurrentPlayerIndex();
            model.addLog(curr->getName() + " chose to go first.");
        } else {
            nextStarter = 1 - model.getCurrentPlayerIndex();
            model.addLog(curr->getName() + " chose opponent to go first.");
        }

        controller.setupAge(model.getCurrentAge() + 1);
        model.setCurrentPlayerIndex(nextStarter);
    }

}