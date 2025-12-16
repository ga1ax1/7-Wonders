#include "GameController.h"
#include "DataLoader.h"
#include <algorithm>
#include <iostream>
#include <random>
#include <chrono>
#include <cmath>

namespace SevenWondersDuel {

    GameController::GameController() {
        auto seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        rng.seed(seed);
        model = std::make_unique<GameModel>();
    }

    void GameController::loadData(const std::string& path) {
        model->allCards.clear();
        model->allWonders.clear();

        bool success = DataLoader::loadFromFile(path, model->allCards, model->allWonders);
        if (!success) {
            std::cerr << "CRITICAL ERROR: Failed to load game data from " << path << std::endl;
            exit(1);
        }
    }

    void GameController::initializeGame(const std::string& jsonPath) {
        loadData(jsonPath);

        model->players.clear();
        model->players.push_back(std::make_unique<Player>(0, "Player 1"));
        model->players.push_back(std::make_unique<Player>(1, "Player 2"));

        model->currentAge = 0;
        model->currentPlayerIndex = 0;
        model->winnerIndex = -1;
        model->victoryType = VictoryType::NONE;
        model->gameLog.clear();

        // 随机标记
        std::vector<ProgressToken> allTokens = {
            ProgressToken::AGRICULTURE, ProgressToken::URBANISM,
            ProgressToken::STRATEGY, ProgressToken::THEOLOGY,
            ProgressToken::ECONOMY, ProgressToken::MASONRY,
            ProgressToken::ARCHITECTURE, ProgressToken::LAW,
            ProgressToken::MATHEMATICS, ProgressToken::PHILOSOPHY
        };

        std::shuffle(allTokens.begin(), allTokens.end(), rng);

        model->board->availableProgressTokens.clear();
        for(int i=0; i<5; ++i) {
            model->board->availableProgressTokens.push_back(allTokens[i]);
        }

        model->board->boxProgressTokens.clear();
        for(size_t i=5; i<allTokens.size(); ++i) {
            model->board->boxProgressTokens.push_back(allTokens[i]);
        }

        model->addLog("[System] Game Initialized. Progress Tokens shuffled.");
    }

    void GameController::startGame() {
        currentState = GameState::WONDER_DRAFT_PHASE_1;
        initWondersDeck();
        dealWondersToDraft();
        model->addLog("[System] Game Started. Wonder Draft Phase 1.");
    }

    void GameController::initWondersDeck() {
        model->remainingWonders.clear();
        for (auto& w : model->allWonders) {
            model->remainingWonders.push_back(&w);
        }
        std::shuffle(model->remainingWonders.begin(), model->remainingWonders.end(), rng);
    }

    void GameController::dealWondersToDraft() {
        model->draftPool.clear();
        for (int i = 0; i < 4 && !model->remainingWonders.empty(); ++i) {
            model->draftPool.push_back(model->remainingWonders.back());
            model->remainingWonders.pop_back();
        }
    }

    // --- 流程控制 ---

    void GameController::setupAge(int age) {
        model->currentAge = age;
        std::vector<Card*> deck = prepareDeckForAge(age);
        model->board->cardStructure.init(age, deck);
        currentState = GameState::AGE_PLAY_PHASE;
        model->addLog("[System] Age " + std::to_string(age) + " Begins!");
    }

    std::vector<Card*> GameController::prepareDeckForAge(int age) {
        std::vector<Card*> deck;
        std::vector<Card*> ageCards;
        std::vector<Card*> guildCards;

        for(auto& c : model->allCards) {
            if (c.type == CardType::GUILD) {
                guildCards.push_back(&c);
            } else if (c.age == age) {
                ageCards.push_back(&c);
            }
        }

        std::shuffle(ageCards.begin(), ageCards.end(), rng);

        if (ageCards.size() > 3) {
            ageCards.resize(ageCards.size() - 3);
        }

        for (auto c : ageCards) deck.push_back(c);

        if (age == 3) {
            std::shuffle(guildCards.begin(), guildCards.end(), rng);
            if (guildCards.size() > 3) {
                guildCards.resize(3);
            }
            for (auto c : guildCards) deck.push_back(c);
            std::shuffle(deck.begin(), deck.end(), rng);
        }

        return deck;
    }

    void GameController::switchPlayer() {
        if (extraTurnPending) {
            extraTurnPending = false;
            model->addLog(">> EXTRA TURN for " + model->getCurrentPlayer()->name);
        } else {
            model->currentPlayerIndex = 1 - model->currentPlayerIndex;
        }
    }

    // --- Actions ---

    void GameController::handleDraftWonder(const Action& action) {
        Player* currPlayer = model->getCurrentPlayer();

        auto it = std::find_if(model->draftPool.begin(), model->draftPool.end(),
            [&](Wonder* w){ return w->id == action.targetWonderId; });

        if (it != model->draftPool.end()) {
            Wonder* w = *it;
            currPlayer->unbuiltWonders.push_back(w);
            model->draftPool.erase(it);

            model->addLog("[" + currPlayer->name + "] drafted wonder: " + w->name);

            switchPlayer();

            if (model->draftPool.empty()) {
                if (currentState == GameState::WONDER_DRAFT_PHASE_1) {
                    currentState = GameState::WONDER_DRAFT_PHASE_2;
                    dealWondersToDraft();
                    model->addLog("[System] Wonder Draft Phase 2 Begins.");
                } else {
                    setupAge(1);
                }
            }
        }
    }

    void GameController::handleBuildCard(const Action& action) {
        Player* currPlayer = model->getCurrentPlayer();
        Player* opponent = model->getOpponent();
        Card* targetCard = findCardInPyramid(action.targetCardId);

        ActionResult res = validateAction(action);
        currPlayer->payCoins(res.cost);

        model->board->cardStructure.removeCard(targetCard->id);
        currPlayer->constructCard(targetCard);

        model->addLog("[" + currPlayer->name + "] built " + targetCard->name);

        for(auto& eff : targetCard->effects) {
            eff->apply(currPlayer, opponent, this);
        }

        // 配对检测 (简化逻辑：若有2个相同符号，触发事件)
        // 实际逻辑应在此处 setState(WAITING_FOR_TOKEN_SELECTION_PAIR)
        // 为了简化流程，暂不中断游戏

        if (currentState == GameState::AGE_PLAY_PHASE) {
             checkVictoryConditions();
             if (currentState != GameState::GAME_OVER) switchPlayer();
        }
    }

    void GameController::handleDiscardCard(const Action& action) {
        Player* currPlayer = model->getCurrentPlayer();
        Card* targetCard = findCardInPyramid(action.targetCardId);

        model->board->cardStructure.removeCard(targetCard->id);
        model->board->discardPile.push_back(targetCard);

        int gain = 2 + currPlayer->getCardCount(CardType::COMMERCIAL);
        currPlayer->gainCoins(gain);

        model->addLog("[" + currPlayer->name + "] discarded " + targetCard->name + " (+ " + std::to_string(gain) + " coins)");
        switchPlayer();
    }

    void GameController::handleBuildWonder(const Action& action) {
        Player* currPlayer = model->getCurrentPlayer();
        Player* opponent = model->getOpponent();
        Card* pyramidCard = findCardInPyramid(action.targetCardId);
        Wonder* wonder = findWonderInHand(currPlayer, action.targetWonderId);

        ActionResult res = validateAction(action);
        currPlayer->payCoins(res.cost);

        model->board->cardStructure.removeCard(pyramidCard->id);
        currPlayer->constructWonder(wonder->id, pyramidCard);

        model->addLog("[" + currPlayer->name + "] built WONDER: " + wonder->name + "!");

        for(auto& eff : wonder->effects) {
            eff->apply(currPlayer, opponent, this);
        }

        int totalBuilt = model->players[0]->builtWonders.size() + model->players[1]->builtWonders.size();
        if (totalBuilt == 7) {
            model->addLog("[System] 7 Wonders built! The 8th wonder is removed.");
            model->players[0]->unbuiltWonders.clear();
            model->players[1]->unbuiltWonders.clear();
        }

        if (currentState == GameState::AGE_PLAY_PHASE) {
            checkVictoryConditions();
            if (currentState != GameState::GAME_OVER) {
                if (!extraTurnPending) switchPlayer();
                else {
                    extraTurnPending = false;
                    model->addLog("[System] EXTRA TURN triggered!");
                }
            }
        }
    }

    void GameController::handleSelectProgressToken(const Action& action) {
        Player* currPlayer = model->getCurrentPlayer();
        ProgressToken token = action.selectedToken;

        std::vector<ProgressToken>* sourcePool = nullptr;
        if (currentState == GameState::WAITING_FOR_TOKEN_SELECTION_PAIR) {
            sourcePool = &model->board->availableProgressTokens;
        } else if (currentState == GameState::WAITING_FOR_TOKEN_SELECTION_LIB) {
            sourcePool = &model->board->boxProgressTokens;
        }

        if (sourcePool) {
            auto it = std::find(sourcePool->begin(), sourcePool->end(), token);
            if (it != sourcePool->end()) {
                sourcePool->erase(it);
                currPlayer->addProgressToken(token);
                model->addLog("[" + currPlayer->name + "] selected a Progress Token.");
                currentState = GameState::AGE_PLAY_PHASE;
            }
        }
    }

    void GameController::handleDestruction(const Action& action) {
        model->addLog("[System] Card destroyed by effect.");
        currentState = GameState::AGE_PLAY_PHASE;
        if (!extraTurnPending) switchPlayer();
        else extraTurnPending = false;
    }

    // --- 校验 ---

    ActionResult GameController::validateAction(const Action& action) {
        ActionResult result;
        result.isValid = false;

        Player* currPlayer = model->getCurrentPlayer();
        Player* opponent = model->getOpponent();

        if (currentState == GameState::WONDER_DRAFT_PHASE_1 ||
            currentState == GameState::WONDER_DRAFT_PHASE_2) {
            if (action.type == ActionType::DRAFT_WONDER) {
                bool found = false;
                for(auto w : model->draftPool) if(w->id == action.targetWonderId) found = true;
                if(found) { result.isValid = true; return result; }
                result.message = "Wonder not available in draft pool";
                return result;
            }
            result.message = "Invalid action for Draft Phase";
            return result;
        }

        if (currentState == GameState::AGE_PLAY_PHASE) {
            Card* target = findCardInPyramid(action.targetCardId);
            if (!target) { result.message = "Card not found"; return result; }

            bool isAvailable = false;
            auto availableSlots = model->board->cardStructure.getAvailableCards();
            for(auto slot : availableSlots) if(slot->cardPtr == target) isAvailable = true;
            if (!isAvailable) { result.message = "Card is currently covered"; return result; }

            if (action.type == ActionType::BUILD_CARD) {
                auto costInfo = currPlayer->calculateCost(target->cost, *opponent);
                if (!target->requiresChainTag.empty() &&
                    currPlayer->ownedChainTags.count(target->requiresChainTag)) {
                    costInfo.first = true; costInfo.second = 0;
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
                Wonder* w = findWonderInHand(currPlayer, action.targetWonderId);
                if (!w) { result.message = "Wonder not found in hand"; return result; }
                if (w->isBuilt) { result.message = "Wonder already built"; return result; }

                auto costInfo = currPlayer->calculateCost(w->cost, *opponent);
                if (!costInfo.first) { result.message = "Insufficient resources for Wonder"; return result; }

                result.isValid = true;
                result.cost = costInfo.second;
                return result;
            }
        }

        result.message = "Unknown action or state";
        return result;
    }

    bool GameController::processAction(const Action& action) {
        ActionResult v = validateAction(action);
        if (!v.isValid) return false;

        switch (action.type) {
            case ActionType::DRAFT_WONDER: handleDraftWonder(action); break;
            case ActionType::BUILD_CARD: handleBuildCard(action); break;
            case ActionType::DISCARD_FOR_COINS: handleDiscardCard(action); break;
            case ActionType::BUILD_WONDER: handleBuildWonder(action); break;
            case ActionType::SELECT_PROGRESS_TOKEN: handleSelectProgressToken(action); break;
            case ActionType::SELECT_DESTRUCTION: handleDestruction(action); break;
            default: return false;
        }
        return true;
    }

    // --- 缺失函数实现 ---

    void GameController::resolveMilitaryLoot(const std::vector<int>& lootEvents) {
        // lootEvents: 正数表示P1推P2(P2丢钱), 负数表示P2推P1(P1丢钱)
        for (int amount : lootEvents) {
            if (amount > 0) {
                // P2 丢钱 (amount: 2 or 5)
                int loss = std::min(model->players[1]->coins, amount);
                model->players[1]->payCoins(loss);
                model->addLog("[Military] Player 2 lost " + std::to_string(loss) + " coins!");
            } else {
                // P1 丢钱
                int loss = std::min(model->players[0]->coins, std::abs(amount));
                model->players[0]->payCoins(loss);
                model->addLog("[Military] Player 1 lost " + std::to_string(loss) + " coins!");
            }
        }
    }

    void GameController::checkVictoryConditions() {
        if (std::abs(model->board->militaryTrack.position) >= 9) {
            currentState = GameState::GAME_OVER;
            // Pos > 0 means P1(Left) pushed to P2(Right). So P1 wins.
            if (model->board->militaryTrack.position > 0) model->winnerIndex = 0;
            else model->winnerIndex = 1;
            model->victoryType = VictoryType::MILITARY;
        }

        for(int i=0; i<2; ++i) {
            int distinctSymbols = 0;
            for(auto const& [sym, count] : model->players[i]->scienceSymbols) {
                if (sym != ScienceSymbol::NONE && count > 0) distinctSymbols++;
            }
            if (distinctSymbols >= 6) {
                currentState = GameState::GAME_OVER;
                model->winnerIndex = i;
                model->victoryType = VictoryType::SCIENCE;
            }
        }
    }

    Card* GameController::findCardInPyramid(const std::string& id) {
        for(auto& c : model->allCards) if (c.id == id) return &c;
        return nullptr;
    }

    Wonder* GameController::findWonderInHand(Player* p, const std::string& id) {
        for(auto w : p->unbuiltWonders) if (w->id == id) return w;
        return nullptr;
    }

}