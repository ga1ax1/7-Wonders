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
        draftTurnCount = 0;
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

    void GameController::prepareNextAge() {
        // 检查是否结束 Age 3 -> 文官胜利
        if (model->currentAge == 3) {
            currentState = GameState::GAME_OVER;
            model->victoryType = VictoryType::CIVILIAN;

            // 计算分数判定胜负
            int s1 = model->players[0]->getScore(*model->players[1]);
            int s2 = model->players[1]->getScore(*model->players[0]);

            if (s1 > s2) model->winnerIndex = 0;
            else if (s2 > s1) model->winnerIndex = 1;
            else {
                // [FIX] 平局判定：蓝卡分
                int blue1 = model->players[0]->getBlueCardScore(*model->players[1]);
                int blue2 = model->players[1]->getBlueCardScore(*model->players[0]);

                if (blue1 > blue2) {
                    model->winnerIndex = 0;
                    model->addLog("[System] Score Tie! Player 1 wins by Civilian (Blue) Points.");
                } else if (blue2 > blue1) {
                    model->winnerIndex = 1;
                    model->addLog("[System] Score Tie! Player 2 wins by Civilian (Blue) Points.");
                } else {
                    model->winnerIndex = -1; // Shared Victory / True Draw
                    model->addLog("[System] True Draw! (Scores and Blue Points identical)");
                }
            }
            return;
        }

        // 准备进入下一时代：判定谁决定先手
        int decisionMaker = -1;
        int pos = model->board->militaryTrack.position;

        if (pos > 0) decisionMaker = 1;
        else if (pos < 0) decisionMaker = 0;
        else decisionMaker = model->currentPlayerIndex; // 刚刚行动完的玩家

        model->currentPlayerIndex = decisionMaker;
        currentState = GameState::WAITING_FOR_START_PLAYER_SELECTION;

        model->addLog("[System] End of Age " + std::to_string(model->currentAge) +
                      ". " + model->getCurrentPlayer()->name + " chooses who starts next age.");
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
        model->currentPlayerIndex = 1 - model->currentPlayerIndex;
    }

    void GameController::onTurnEnd() {
        // 1. 检查胜利条件 (军事/科技)
        checkVictoryConditions();
        if (currentState == GameState::GAME_OVER) return;

        // 2. 检查是否时代结束 (金字塔空了)
        if (model->getRemainingCardCount() == 0) {
            prepareNextAge();
            return;
        }

        // 3. 正常回合切换
        if (extraTurnPending) {
            extraTurnPending = false;
            model->addLog(">> EXTRA TURN for " + model->getCurrentPlayer()->name);
        } else {
            switchPlayer();
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

            // 1-2-1 逻辑
            bool shouldSwitch = true;
            if (draftTurnCount == 1) shouldSwitch = false;

            draftTurnCount++;

            if (model->draftPool.empty()) {
                if (currentState == GameState::WONDER_DRAFT_PHASE_1) {
                    currentState = GameState::WONDER_DRAFT_PHASE_2;
                    dealWondersToDraft();
                    draftTurnCount = 0;
                    model->currentPlayerIndex = 1;
                    model->addLog("[System] Wonder Draft Phase 2 Begins. Player 2 starts.");
                } else {
                    setupAge(1);
                    model->currentPlayerIndex = 0;
                }
            } else {
                if (shouldSwitch) {
                    model->currentPlayerIndex = 1 - model->currentPlayerIndex;
                }
            }
        }
    }

    void GameController::handleBuildCard(const Action& action) {
        Player* currPlayer = model->getCurrentPlayer();
        Player* opponent = model->getOpponent();
        Card* targetCard = findCardInPyramid(action.targetCardId);

        // [UPDATED] 传入 CardType 用于计算减费
        ActionResult res = validateAction(action);
        currPlayer->payCoins(res.cost);

        model->board->cardStructure.removeCard(targetCard->id);
        currPlayer->constructCard(targetCard);

        model->addLog("[" + currPlayer->name + "] built " + targetCard->name);

        // [FIX] Urbanism Check: 如果是连锁建造 (res.cost == 0 但 原卡有消费 且 确实是 Chain)，奖励4元
        bool isChain = false;
        if (!targetCard->requiresChainTag.empty() &&
            currPlayer->ownedChainTags.count(targetCard->requiresChainTag)) {
            isChain = true;
        }

        if (isChain && currPlayer->progressTokens.count(ProgressToken::URBANISM)) {
            currPlayer->gainCoins(4);
            model->addLog("[Effect] Urbanism: +4 coins from chain build.");
        }

        for(auto& eff : targetCard->effects) {
            eff->apply(currPlayer, opponent, this);
        }

        if (checkForNewSciencePairs(currPlayer)) {
            return;
        }

        if (currentState == GameState::AGE_PLAY_PHASE) {
             onTurnEnd();
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

        onTurnEnd();
    }

    void GameController::handleBuildWonder(const Action& action) {
        Player* currPlayer = model->getCurrentPlayer();
        Player* opponent = model->getOpponent();
        Card* pyramidCard = findCardInPyramid(action.targetCardId);
        Wonder* wonder = findWonderInHand(currPlayer, action.targetWonderId);

        // [UPDATED] 传入 CardType::WONDER
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

        if (currPlayer->progressTokens.count(ProgressToken::THEOLOGY)) {
             grantExtraTurn();
             model->addLog("[Effect] Theology Token grants an Extra Turn!");
        }

        if (checkForNewSciencePairs(currPlayer)) {
            return;
        }

        if (currentState == GameState::AGE_PLAY_PHASE) {
            onTurnEnd();
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

                // [FIX] Urbanism Instant Bonus
                if (token == ProgressToken::URBANISM) {
                    currPlayer->gainCoins(6);
                    model->addLog("[Effect] Urbanism: +6 coins immediately.");
                }

                currentState = GameState::AGE_PLAY_PHASE;

                if (token == ProgressToken::LAW) {
                    if (checkForNewSciencePairs(currPlayer)) {
                        return;
                    }
                }

                onTurnEnd();
            }
        }
    }

    void GameController::handleDestruction(const Action& action) {
        if (action.targetCardId.empty()) {
            model->addLog("[System] Destruction skipped.");
            currentState = GameState::AGE_PLAY_PHASE;
            onTurnEnd();
            return;
        }

        Player* opponent = model->getOpponent();
        Card* target = nullptr;
        for(auto c : opponent->builtCards) {
            if (c->id == action.targetCardId) {
                target = c; break;
            }
        }

        if (target) {
             model->board->destroyCard(opponent, target->type);
             model->addLog("[System] " + opponent->name + "'s card " + target->name + " destroyed.");
        }

        currentState = GameState::AGE_PLAY_PHASE;
        onTurnEnd();
    }

    void GameController::handleChooseStartingPlayer(const Action& action) {
        Player* curr = model->getCurrentPlayer();

        int nextStarter = -1;
        if (action.targetCardId == "ME") {
            nextStarter = model->currentPlayerIndex;
            model->addLog(curr->name + " chose to go first.");
        } else {
            nextStarter = 1 - model->currentPlayerIndex;
            model->addLog(curr->name + " chose opponent to go first.");
        }

        setupAge(model->currentAge + 1);
        model->currentPlayerIndex = nextStarter;
    }

    void GameController::handleSelectFromDiscard(const Action& action) {
        Player* currPlayer = model->getCurrentPlayer();
        Player* opponent = model->getOpponent();

        auto& pile = model->board->discardPile;
        auto it = std::find_if(pile.begin(), pile.end(), [&](Card* c){ return c->id == action.targetCardId; });

        if (it != pile.end()) {
            Card* card = *it;
            pile.erase(it);
            currPlayer->constructCard(card);

            model->addLog("[" + currPlayer->name + "] resurrected " + card->name + " from discard!");

            for(auto& eff : card->effects) {
                eff->apply(currPlayer, opponent, this);
            }

             if (checkForNewSciencePairs(currPlayer)) {
                return;
            }
        }

        currentState = GameState::AGE_PLAY_PHASE;
        onTurnEnd();
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

        if (currentState == GameState::WAITING_FOR_TOKEN_SELECTION_PAIR ||
            currentState == GameState::WAITING_FOR_TOKEN_SELECTION_LIB) {
            if (action.type == ActionType::SELECT_PROGRESS_TOKEN) {
                result.isValid = true; return result;
            }
            result.message = "Must select a Progress Token";
            return result;
        }

        if (currentState == GameState::WAITING_FOR_DESTRUCTION) {
            if (action.type == ActionType::SELECT_DESTRUCTION) {
                if (action.targetCardId.empty()) {
                    result.isValid = true;
                    return result;
                }

                Player* opponent = model->getOpponent();
                bool hasCard = false;
                Card* targetCard = nullptr;
                for (auto c : opponent->builtCards) {
                    if (c->id == action.targetCardId) {
                        hasCard = true;
                        targetCard = c;
                        break;
                    }
                }

                if (!hasCard || !targetCard) {
                    result.message = "Opponent does not possess this card";
                    return result;
                }

                if (targetCard->type != pendingDestructionType) {
                    result.message = "Invalid card color. Must destroy a specific type.";
                    return result;
                }

                result.isValid = true;
                return result;
            }
            result.message = "Must select a card to destroy (or empty to skip)";
            return result;
        }

        if (currentState == GameState::WAITING_FOR_DISCARD_BUILD) {
             if (action.type == ActionType::SELECT_FROM_DISCARD) {
                 auto& pile = model->board->discardPile;
                 auto it = std::find_if(pile.begin(), pile.end(), [&](Card* c){ return c->id == action.targetCardId; });
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

        if (currentState == GameState::WAITING_FOR_START_PLAYER_SELECTION) {
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

        if (currentState == GameState::AGE_PLAY_PHASE) {
            Card* target = findCardInPyramid(action.targetCardId);
            if (!target) { result.message = "Card not found"; return result; }

            bool isAvailable = false;
            auto availableSlots = model->board->cardStructure.getAvailableCards();
            for(auto slot : availableSlots) if(slot->cardPtr == target) isAvailable = true;
            if (!isAvailable) { result.message = "Card is currently covered"; return result; }

            if (action.type == ActionType::BUILD_CARD) {
                // [UPDATED] 传入 target->type
                auto costInfo = currPlayer->calculateCost(target->cost, *opponent, target->type);

                // 检查连锁
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

                // [UPDATED] 传入 CardType::WONDER
                auto costInfo = currPlayer->calculateCost(w->cost, *opponent, CardType::WONDER);
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
            case ActionType::CHOOSE_STARTING_PLAYER: handleChooseStartingPlayer(action); break;
            case ActionType::SELECT_FROM_DISCARD: handleSelectFromDiscard(action); break;
            default: return false;
        }
        return true;
    }

    // --- 缺失函数实现 ---

    bool GameController::checkForNewSciencePairs(Player* p) {
        for (auto const& [sym, count] : p->scienceSymbols) {
            if (sym == ScienceSymbol::NONE) continue;

            if (count >= 2) {
                if (p->claimedSciencePairs.find(sym) == p->claimedSciencePairs.end()) {
                    p->claimedSciencePairs.insert(sym);
                    setState(GameState::WAITING_FOR_TOKEN_SELECTION_PAIR);
                    model->addLog(p->name + " collected a Science Pair! Choose a Progress Token.");
                    return true;
                }
            }
        }
        return false;
    }

    void GameController::resolveMilitaryLoot(const std::vector<int>& lootEvents) {
        for (int amount : lootEvents) {
            if (amount > 0) {
                int loss = std::min(model->players[1]->coins, amount);
                model->players[1]->payCoins(loss);
                model->addLog("[Military] Player 2 lost " + std::to_string(loss) + " coins!");
            } else {
                int loss = std::min(model->players[0]->coins, std::abs(amount));
                model->players[0]->payCoins(loss);
                model->addLog("[Military] Player 1 lost " + std::to_string(loss) + " coins!");
            }
        }
    }

    void GameController::checkVictoryConditions() {
        if (std::abs(model->board->militaryTrack.position) >= 9) {
            currentState = GameState::GAME_OVER;
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