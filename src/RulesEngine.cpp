#include "RulesEngine.h"
#include <cmath>

namespace SevenWondersDuel {

    VictoryResult RulesEngine::checkInstantVictory(const Player& p1, const Player& p2, const Board& board) {
        VictoryResult result;

        // 1. Military Supremacy
        int pos = board.getMilitaryTrack().getPosition();
        if (std::abs(pos) >= Config::MILITARY_THRESHOLD_WIN) {
            result.isGameOver = true;
            result.type = VictoryType::MILITARY;
            // pos > 0 means P1 (index 0) pushed towards P2
            // pos < 0 means P2 (index 1) pushed towards P1
            // P0 (Id=0) pushes positive (+). P1 (Id=1) pushes negative (-).
            // If pos >= 9 (Positive extreme), P0 (Player 1) wins.
            // If pos <= -9 (Negative extreme), P1 (Player 2) wins.
            
            if (pos > 0) result.winnerIndex = 0;
            else result.winnerIndex = 1;
            
            return result;
        }

        // 2. Science Supremacy
        const Player* players[2] = { &p1, &p2 };
        for (int i = 0; i < 2; ++i) {
            int distinctSymbols = 0;
            for (auto const& [sym, count] : players[i]->getScienceSymbols()) {
                if (sym != ScienceSymbol::NONE && count > 0) {
                    distinctSymbols++;
                }
            }
            if (distinctSymbols >= Config::SCIENCE_WIN_THRESHOLD) {
                result.isGameOver = true;
                result.type = VictoryType::SCIENCE;
                result.winnerIndex = i;
                return result;
            }
        }

        return result;
    }

}
