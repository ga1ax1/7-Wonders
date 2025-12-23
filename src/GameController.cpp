#include "GameController.h"
#include "DataLoader.h"
#include "RulesEngine.h"
#include "ScoringManager.h"
#include "GameStateLogic.h"
#include "GameCommands.h"
#include <algorithm>
#include <iostream>
#include <random>
#include <chrono>
#include <cmath>

namespace SevenWondersDuel {

    GameController::GameController() {
        auto seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        m_rng.seed(seed);
        m_model = std::make_unique<GameModel>();
        updateStateLogic(GameState::WONDER_DRAFT_PHASE_1);
    }

    GameController::~GameController() = default;

    void GameController::updateStateLogic(GameState newState) {
        switch (newState) {
            case GameState::WONDER_DRAFT_PHASE_1:
            case GameState::WONDER_DRAFT_PHASE_2:
                m_stateLogic = std::make_unique<WonderDraftState>();
                break;
            case GameState::AGE_PLAY_PHASE:
                m_stateLogic = std::make_unique<AgePlayState>();
                break;
            case GameState::WAITING_FOR_TOKEN_SELECTION_PAIR:
            case GameState::WAITING_FOR_TOKEN_SELECTION_LIB:
                m_stateLogic = std::make_unique<TokenSelectionState>();
                break;
            case GameState::WAITING_FOR_DESTRUCTION:
                m_stateLogic = std::make_unique<DestructionState>();
                break;
            case GameState::WAITING_FOR_DISCARD_BUILD:
                m_stateLogic = std::make_unique<DiscardBuildState>();
                break;
            case GameState::WAITING_FOR_START_PLAYER_SELECTION:
                m_stateLogic = std::make_unique<StartPlayerSelectionState>();
                break;
            case GameState::GAME_OVER:
                m_stateLogic = std::make_unique<GameOverState>();
                break;
        }
        if (m_stateLogic) {
            m_stateLogic->onEnter(*this);
        }
    }

    void GameController::loadData(const std::string& path) {
        std::vector<Card> cards;
        std::vector<Wonder> wonders;

        bool success = DataLoader::loadFromFile(path, cards, wonders);
        if (!success) {
            std::cerr << "CRITICAL ERROR: Failed to load game data from " << path << std::endl;
            exit(1);
        }
        m_model->populateData(std::move(cards), std::move(wonders));
    }

    void GameController::initializeGame(const std::string& jsonPath) {
        loadData(jsonPath);

        m_model->clearPlayers();
        m_model->addPlayer(std::make_unique<Player>(0, "Player 1"));
        m_model->addPlayer(std::make_unique<Player>(1, "Player 2"));

        m_model->setCurrentAge(0);
        m_model->setCurrentPlayerIndex(0);
        m_model->setWinnerIndex(-1);
        m_model->setVictoryType(VictoryType::NONE);
        m_model->clearLog();

        std::vector<ProgressToken> allTokens = {
            ProgressToken::AGRICULTURE, ProgressToken::URBANISM,
            ProgressToken::STRATEGY, ProgressToken::THEOLOGY,
            ProgressToken::ECONOMY, ProgressToken::MASONRY,
            ProgressToken::ARCHITECTURE, ProgressToken::LAW,
            ProgressToken::MATHEMATICS, ProgressToken::PHILOSOPHY
        };

        std::shuffle(allTokens.begin(), allTokens.end(), m_rng);

        m_model->getBoardMut()->setAvailableProgressTokens({});
        for(int i=0; i<5; ++i) {
            m_model->getBoardMut()->addAvailableProgressToken(allTokens[i]);
        }

        m_model->getBoardMut()->setBoxProgressTokens({});
        for(size_t i=5; i<allTokens.size(); ++i) {
            m_model->getBoardMut()->addBoxProgressToken(allTokens[i]);
        }

        m_model->addLog("[System] Game Initialized. Progress Tokens shuffled.");
    }

    void GameController::startGame() {
        setState(GameState::WONDER_DRAFT_PHASE_1);
        m_draftTurnCount = 0;
        initWondersDeck();
        dealWondersToDraft();
        m_model->addLog("[System] Game Started. Wonder Draft Phase 1.");
    }

    GameState GameController::getState() const {
        return m_currentState;
    }

    const GameModel& GameController::getModel() const {
        return *m_model;
    }

    void GameController::initWondersDeck() {
        m_model->clearRemainingWonders();
        std::vector<Wonder*> temp = m_model->getPointersToAllWonders();
        std::shuffle(temp.begin(), temp.end(), m_rng);
        for (auto w : temp) {
            m_model->addToRemainingWonders(w);
        }
    }

    void GameController::dealWondersToDraft() {
        m_model->clearDraftPool();
        for (int i = 0; i < 4; ++i) {
            Wonder* w = m_model->backRemainingWonder();
            if (w) {
                m_model->addToDraftPool(w);
                m_model->popRemainingWonder();
            }
        }
    }

    void GameController::setupAge(int age) {
        m_model->setCurrentAge(age);
        std::vector<Card*> deck = prepareDeckForAge(age);
        m_model->getBoardMut()->initPyramid(age, deck);
        setState(GameState::AGE_PLAY_PHASE);
        m_model->addLog("[System] Age " + std::to_string(age) + " Begins!");
    }

    void GameController::prepareNextAge() {
        if (m_model->getCurrentAge() == 3) {
            setState(GameState::GAME_OVER);
            m_model->setVictoryType(VictoryType::CIVILIAN);

            int s1 = ScoringManager::calculateScore(*m_model->getPlayers()[0], *m_model->getPlayers()[1], *m_model->getBoard());
            int s2 = ScoringManager::calculateScore(*m_model->getPlayers()[1], *m_model->getPlayers()[0], *m_model->getBoard());

            if (s1 > s2) m_model->setWinnerIndex(0);
            else if (s2 > s1) m_model->setWinnerIndex(1);
            else {
                int blue1 = ScoringManager::calculateBluePoints(*m_model->getPlayers()[0], *m_model->getPlayers()[1]);
                int blue2 = ScoringManager::calculateBluePoints(*m_model->getPlayers()[1], *m_model->getPlayers()[0]);

                if (blue1 > blue2) {
                    m_model->setWinnerIndex(0);
                    m_model->addLog("[System] Score Tie! Player 1 wins by Civilian (Blue) Points.");
                } else if (blue2 > blue1) {
                    m_model->setWinnerIndex(1);
                    m_model->addLog("[System] Score Tie! Player 2 wins by Civilian (Blue) Points.");
                } else {
                    m_model->setWinnerIndex(-1);
                    m_model->addLog("[System] True Draw! (Scores and Blue Points identical)");
                }
            }
            return;
        }

        int decisionMaker = -1;
        int pos = m_model->getBoard()->getMilitaryTrack().getPosition();

        if (pos > 0) decisionMaker = 1;
        else if (pos < 0) decisionMaker = 0;
        else decisionMaker = m_model->getCurrentPlayerIndex();

        m_model->setCurrentPlayerIndex(decisionMaker);
        setState(GameState::WAITING_FOR_START_PLAYER_SELECTION);

        m_model->addLog("[System] End of Age " + std::to_string(m_model->getCurrentAge()) +
                      ". " + m_model->getCurrentPlayer()->getName() + " chooses who starts next age.");
    }

    std::vector<Card*> GameController::prepareDeckForAge(int age) {
        std::vector<Card*> deck;
        std::vector<Card*> ageCards;
        std::vector<Card*> guildCards;

        for(const auto& c_const : m_model->getAllCards()) {
            Card* c = m_model->findCardById(c_const.getId());
            if (c->getType() == CardType::GUILD) {
                guildCards.push_back(c);
            } else if (c->getAge() == age) {
                ageCards.push_back(c);
            }
        }

        std::shuffle(ageCards.begin(), ageCards.end(), m_rng);

        if (ageCards.size() > 3) {
            ageCards.resize(ageCards.size() - 3);
        }

        for (auto c : ageCards) deck.push_back(c);

        if (age == 3) {
            std::shuffle(guildCards.begin(), guildCards.end(), m_rng);
            if (guildCards.size() > 3) {
                guildCards.resize(3);
            }
            for (auto c : guildCards) deck.push_back(c);
            std::shuffle(deck.begin(), deck.end(), m_rng);
        }

        return deck;
    }

    void GameController::switchPlayer() {
        m_model->setCurrentPlayerIndex(1 - m_model->getCurrentPlayerIndex());
    }

    void GameController::onTurnEnd() {
        checkVictoryConditions();
        if (m_currentState == GameState::GAME_OVER) return;

        if (m_model->getRemainingCardCount() == 0) {
            prepareNextAge();
            return;
        }

        if (m_extraTurnPending) {
            m_extraTurnPending = false;
            m_model->addLog(">> EXTRA TURN for " + m_model->getCurrentPlayer()->getName());
        } else {
            switchPlayer();
        }
    }

    void GameController::setState(GameState newState) {
        m_currentState = newState;
        updateStateLogic(newState);
    }

    ActionResult GameController::validateAction(const Action& action) {
        if (m_stateLogic) {
            return m_stateLogic->validate(action, *this);
        }
        return {false, 0, "State Logic not initialized"};
    }

    bool GameController::processAction(const Action& action) {
        ActionResult v = validateAction(action);
        if (!v.isValid) return false;

        auto cmd = CommandFactory::createCommand(action);
        if (cmd) {
            cmd->execute(*this);
            return true;
        }
        return false;
    }

    bool GameController::checkForNewSciencePairs(Player* p) {
        for (auto const& [sym, count] : p->getScienceSymbols()) {
            if (sym == ScienceSymbol::NONE) continue;

            if (count >= Config::SCIENCE_PAIR_COUNT) {
                if (p->getClaimedSciencePairs().find(sym) == p->getClaimedSciencePairs().end()) {
                    p->addClaimedSciencePair(sym);
                    setState(GameState::WAITING_FOR_TOKEN_SELECTION_PAIR);
                    m_model->addLog(p->getName() + " collected a Science Pair! Choose a Progress Token.");
                    return true;
                }
            }
        }
        return false;
    }

    void GameController::resolveMilitaryLoot(const std::vector<int>& lootEvents) {
        for (int amount : lootEvents) {
            if (amount > 0) {
                int loss = std::min(m_model->getPlayers()[1]->getCoins(), amount);
                m_model->getPlayers()[1]->payCoins(loss);
                m_model->addLog("[Military] Player 2 lost " + std::to_string(loss) + " coins!");
            } else {
                int loss = std::min(m_model->getPlayers()[0]->getCoins(), std::abs(amount));
                m_model->getPlayers()[0]->payCoins(loss);
                m_model->addLog("[Military] Player 1 lost " + std::to_string(loss) + " coins!");
            }
        }
    }

    void GameController::checkVictoryConditions() {
         VictoryResult result = RulesEngine::checkInstantVictory(*m_model->getPlayers()[0], *m_model->getPlayers()[1], *m_model->getBoard());
        if (result.isGameOver) {
            setState(GameState::GAME_OVER);
            m_model->setWinnerIndex(result.winnerIndex);
            m_model->setVictoryType(result.type);
        }
    }

    Card* GameController::findCardInPyramid(const std::string& id) {
        return m_model->findCardById(id);
    }

    Wonder* GameController::findWonderInHand(const Player* p, const std::string& id) {
        for(auto w : p->getUnbuiltWonders()) if (w->getId() == id) return w;
        return nullptr;
    }

    std::vector<int> GameController::moveMilitary(int shields, int playerId) {
        return m_model->getBoardMut()->moveMilitary(shields, playerId);
    }

    bool GameController::isDiscardPileEmpty() const {
        return m_model->getBoard()->getDiscardPile().empty();
    }

}