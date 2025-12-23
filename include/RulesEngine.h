#ifndef SEVEN_WONDERS_DUEL_RULESENGINE_H
#define SEVEN_WONDERS_DUEL_RULESENGINE_H

#include "Global.h"
#include "Player.h"
#include "Board.h"

namespace SevenWondersDuel {

    struct VictoryResult {
        bool isGameOver = false;
        VictoryType type = VictoryType::NONE;
        int winnerIndex = -1;
    };

    class RulesEngine {
    public:
        // Checks for Military and Science Supremacy
        static VictoryResult checkInstantVictory(const Player& p1, const Player& p2, const Board& board);
    };

}

#endif // SEVEN_WONDERS_DUEL_RULESENGINE_H
