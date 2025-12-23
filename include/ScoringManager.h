#ifndef SEVEN_WONDERS_DUEL_SCORINGMANAGER_H
#define SEVEN_WONDERS_DUEL_SCORINGMANAGER_H

#include "Player.h"
#include "Board.h"

namespace SevenWondersDuel {

    class ScoringManager {
    public:
        // Calculate total victory points for a player
        static int calculateScore(const Player& player, const Player& opponent, const Board& board);

        // Calculate civilian (blue card) points for tie-breaking
        static int calculateBluePoints(const Player& player, const Player& opponent);
    };

}

#endif // SEVEN_WONDERS_DUEL_SCORINGMANAGER_H
