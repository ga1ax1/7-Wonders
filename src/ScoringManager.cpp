#include "ScoringManager.h"

namespace SevenWondersDuel {

    int ScoringManager::calculateScore(const Player& player, const Player& opponent, const Board& board) {
        int score = 0;

        // 1. Cards (including Guilds)
        for (const auto& card : player.getBuiltCards()) {
            score += card->getVictoryPoints(&player, &opponent);
        }

        // 2. Wonders
        for (const auto& wonder : player.getBuiltWonders()) {
            score += wonder->getVictoryPoints(&player, &opponent);
        }

        // 3. Military Track
        score += board.getMilitaryTrack().getVictoryPoints(player.getId());

        // 4. Coins (3 coins = 1 VP)
        score += player.getCoins() / Config::COINS_PER_VP;

        // 5. Progress Tokens
        for (auto token : player.getProgressTokens()) {
            if (token == ProgressToken::AGRICULTURE) score += Config::AGRICULTURE_VP;
            if (token == ProgressToken::MATHEMATICS) score += Config::MATHEMATICS_VP_PER_TOKEN * player.getProgressTokens().size(); 
            if (token == ProgressToken::PHILOSOPHY) score += Config::PHILOSOPHY_VP;
        }

        return score;
    }

    int ScoringManager::calculateBluePoints(const Player& player, const Player& opponent) {
        int score = 0;
        for (const auto& card : player.getBuiltCards()) {
            if (card->getType() == CardType::CIVILIAN) {
                score += card->getVictoryPoints(&player, &opponent);
            }
        }
        return score;
    }

}
