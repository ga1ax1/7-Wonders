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

    class IGameStateLogic;

    // 模型聚合根 (Model Layer Root)
    class GameModel {
    private:
        std::vector<std::unique_ptr<Player>> m_players;
        std::unique_ptr<Board> m_board;

        int m_currentAge = 0;
        int m_currentPlayerIndex = 0; // 0 或 1
        int m_winnerIndex = -1;       // -1:未结束, 0:P1胜, 1:P2胜
        VictoryType m_victoryType = VictoryType::NONE;

        std::vector<Wonder*> m_draftPool;        // 当前轮抽区可见的4张
        std::vector<Wonder*> m_remainingWonders; // 剩下的奇迹（保证不重复）

        std::vector<Card> m_allCards;
        std::vector<Wonder> m_allWonders;

        std::vector<std::string> m_gameLog;

    public:
        GameModel();

        // --- Getters (Read-Only) ---

        const Player* getCurrentPlayer() const { return m_players[m_currentPlayerIndex].get(); }
        const Player* getOpponent() const { return m_players[1 - m_currentPlayerIndex].get(); }
        
        const Board* getBoard() const { return m_board.get(); }
        const std::vector<std::unique_ptr<Player>>& getPlayers() const { return m_players; }
        
        int getCurrentAge() const { return m_currentAge; }
        int getCurrentPlayerIndex() const { return m_currentPlayerIndex; }
        int getWinnerIndex() const { return m_winnerIndex; }
        VictoryType getVictoryType() const { return m_victoryType; }

        const std::vector<Wonder*>& getDraftPool() const { return m_draftPool; }
        const std::vector<Wonder*>& getRemainingWonders() const { return m_remainingWonders; }
        const std::vector<Card>& getAllCards() const { return m_allCards; }
        const std::vector<Wonder>& getAllWonders() const { return m_allWonders; }
        const std::vector<std::string>& getGameLog() const { return m_gameLog; }

        // --- Mutators (Controlled) ---
        
        Player* getCurrentPlayerMut() { return m_players[m_currentPlayerIndex].get(); }
        Player* getOpponentMut() { return m_players[1 - m_currentPlayerIndex].get(); }
        Board* getBoardMut() { return m_board.get(); }

        void clearPlayers() { m_players.clear(); }
        void addPlayer(std::unique_ptr<Player> p) { m_players.push_back(std::move(p)); }
        
        void setCurrentAge(int age) { m_currentAge = age; }
        void setCurrentPlayerIndex(int index) { m_currentPlayerIndex = index; }
        void setWinnerIndex(int index) { m_winnerIndex = index; }
        void setVictoryType(VictoryType type) { m_victoryType = type; }

        void clearDraftPool() { m_draftPool.clear(); }
        void addToDraftPool(Wonder* w) { m_draftPool.push_back(w); }
        void removeFromDraftPool(const std::string& wonderId);

        void clearRemainingWonders() { m_remainingWonders.clear(); }
        void addToRemainingWonders(Wonder* w) { m_remainingWonders.push_back(w); }
        void popRemainingWonder();
        Wonder* backRemainingWonder();

        void populateData(std::vector<Card> cards, std::vector<Wonder> wonders);

        Card* findCardById(const std::string& id);
        const Card* findCardById(const std::string& id) const;
        Wonder* findWonderById(const std::string& id);
        const Wonder* findWonderById(const std::string& id) const;

        std::vector<Wonder*> getPointersToAllWonders();

        void addLog(const std::string& msg);
        void clearLog();

        int getRemainingCardCount() const;
    };

    // 游戏控制器
    class GameController : public ILogger, public IGameActions {
        friend class DraftWonderCommand;
        friend class BuildCardCommand;
        friend class DiscardCardCommand;
        friend class BuildWonderCommand;
        friend class SelectProgressTokenCommand;
        friend class DestructionCommand;
        friend class SelectFromDiscardCommand;
        friend class ChooseStartingPlayerCommand;

    public:
        GameController();
        ~GameController();

        void initializeGame(const std::string& jsonPath, const std::string& p1Name, const std::string& p2Name);
        void startGame();

        GameState getState() const;
        const GameModel& getModel() const;

        ActionResult validateAction(const Action& action);
        bool processAction(const Action& action);

        void setPendingDestructionType(CardType t) override { m_pendingDestructionType = t; }
        CardType getPendingDestructionType() const { return m_pendingDestructionType; }

        void setState(GameState newState) override;
        std::vector<int> moveMilitary(int shields, int playerId) override;
        bool isDiscardPileEmpty() const override;
        void grantExtraTurn() override { m_extraTurnPending = true; }
        void addLog(const std::string& msg) override { m_model->addLog(msg); }

    private:
        std::unique_ptr<GameModel> m_model;
        std::unique_ptr<IGameStateLogic> m_stateLogic;
        GameState m_currentState = GameState::WONDER_DRAFT_PHASE_1;

        bool m_extraTurnPending = false;
        int m_draftTurnCount = 0; 

        std::mt19937 m_rng; 

        CardType m_pendingDestructionType = CardType::CIVILIAN;

        void updateStateLogic(GameState newState);
        void loadData(const std::string& path);
        void setupAge(int age);
        void prepareNextAge();
        std::vector<Card*> prepareDeckForAge(int age);
        void initWondersDeck();
        void dealWondersToDraft();
        void onTurnEnd();
        void switchPlayer();
        void checkVictoryConditions();
        void resolveMilitaryLoot(const std::vector<int>& lootEvents);
        bool checkForNewSciencePairs(Player* p);
        Card* findCardInPyramid(const std::string& id);
        Wonder* findWonderInHand(const Player* p, const std::string& id);
    };
}

#endif // SEVEN_WONDERS_DUEL_GAMECONTROLLER_H
