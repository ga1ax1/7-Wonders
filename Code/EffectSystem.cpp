//
// Created by choyichi on 2025/12/15.
//

#include "EffectSystem.h"
#include "Player.h"
#include "GameController.h"
#include <algorithm>
#include <sstream>

namespace SevenWondersDuel {

    // --- 1. ProductionEffect ---
    void ProductionEffect::apply(Player* self, Player* opponent, GameController* ctx) {
        if (isChoice) {
            std::vector<ResourceType> choices;
            for (auto const& [type, count] : producedResources) {
                choices.push_back(type);
            }
            self->addProductionChoice(choices);
        } else {
            for (auto const& [type, count] : producedResources) {
                self->addResource(type, count);
            }
        }
    }

    std::string ProductionEffect::getDescription() const {
        std::stringstream ss;
        ss << "Produces ";
        bool first = true;
        for (auto const& [type, count] : producedResources) {
            if (!first) ss << (isChoice ? " OR " : " AND ");
            ss << count << " " << resourceToString(type);
            first = false;
        }
        return ss.str();
    }

    // --- 2. MilitaryEffect ---
    void MilitaryEffect::apply(Player* self, Player* opponent, GameController* ctx) {
        int finalShields = shields;

        // 规则修复：Strategy Token 仅对军事建筑 (Red Cards) 生效，+1 盾
        if (isFromCard && self->progressTokens.count(ProgressToken::STRATEGY)) {
            finalShields += 1;
            ctx->addLog("[Effect] Strategy Token adds +1 Shield.");
        }

        auto lootEvents = ctx->getBoard()->militaryTrack.move(finalShields, self->id);

        for (int amount : lootEvents) {
            // 扣对手的钱
            int loss = std::abs(amount);
            opponent->payCoins(loss);
            ctx->addLog("[Military] Opponent lost " + std::to_string(loss) + " coins!");
        }
    }

    std::string MilitaryEffect::getDescription() const {
        return "Shields: " + std::to_string(shields);
    }

    // --- 3. ScienceEffect ---
    void ScienceEffect::apply(Player* self, Player* opponent, GameController* ctx) {
        self->addScienceSymbol(symbol);
        // 配对逻辑已在 GameController::handleBuildCard 中通过 checkForNewSciencePairs 统一处理
    }

    std::string ScienceEffect::getDescription() const {
        return "Science Symbol";
    }

    // --- 4. VictoryPointEffect ---
    void VictoryPointEffect::apply(Player* self, Player* opponent, GameController* ctx) {
        // 立即效果无，只计分
    }

    int VictoryPointEffect::calculateScore(const Player* self, const Player* opponent) const {
        return points;
    }

    std::string VictoryPointEffect::getDescription() const {
        return "VP: " + std::to_string(points);
    }

    // --- 5. CoinEffect ---
    void CoinEffect::apply(Player* self, Player* opponent, GameController* ctx) {
        self->gainCoins(amount);
    }

    std::string CoinEffect::getDescription() const {
        return "Coins: " + std::to_string(amount);
    }

    // --- 6. CoinsPerTypeEffect (商业/行会) ---
    void CoinsPerTypeEffect::apply(Player* self, Player* opponent, GameController* ctx) {
        int count = 0;
        count += self->getCardCount(targetType);

        if (countWonder) {
            count += self->builtWonders.size();
        }

        int bonus = count * coinsPerCard;
        self->gainCoins(bonus);
    }

    std::string CoinsPerTypeEffect::getDescription() const {
        return "Coins per card type: " + std::to_string(coinsPerCard);
    }

    // --- 7. TradeDiscountEffect ---
    void TradeDiscountEffect::apply(Player* self, Player* opponent, GameController* ctx) {
        self->tradingDiscounts[resource] = true;
    }

    std::string TradeDiscountEffect::getDescription() const {
        return "Fixed trading price (1 coin) for " + resourceToString(resource);
    }

    // --- 8. DestroyCardEffect ---
    void DestroyCardEffect::apply(Player* self, Player* opponent, GameController* ctx) {
        ctx->setState(GameState::WAITING_FOR_DESTRUCTION);
    }

    std::string DestroyCardEffect::getDescription() const {
        return "Destroy an opponent's card of specific color.";
    }

    // --- 9. ExtraTurnEffect ---
    void ExtraTurnEffect::apply(Player* self, Player* opponent, GameController* ctx) {
        ctx->grantExtraTurn();
    }

    // --- 10. BuildFromDiscardEffect ---
    void BuildFromDiscardEffect::apply(Player* self, Player* opponent, GameController* ctx) {
        ctx->setState(GameState::WAITING_FOR_DISCARD_BUILD);
    }

    // --- 11. ProgressTokenSelectEffect ---
    void ProgressTokenSelectEffect::apply(Player* self, Player* opponent, GameController* ctx) {
        ctx->setState(GameState::WAITING_FOR_TOKEN_SELECTION_LIB);
    }

    // --- 12. OpponentLoseCoinsEffect ---
    void OpponentLoseCoinsEffect::apply(Player* self, Player* opponent, GameController* ctx) {
        int loss = std::min(opponent->coins, amount);
        opponent->payCoins(loss);
    }
    std::string OpponentLoseCoinsEffect::getDescription() const {
        return "Opponent loses " + std::to_string(amount) + " coins.";
    }

    // --- 13. GuildEffect ---
    void GuildEffect::apply(Player* self, Player* opponent, GameController* ctx) {
        // 立即效果：获得金币 (通常是每张1元)
        int count = 0;
        int selfCount = 0;
        int oppCount = 0;
        int multiplier = 1; // Default 1 coin per item

        switch (criteria) {
            case GuildCriteria::YELLOW_CARDS:
                selfCount = self->getCardCount(CardType::COMMERCIAL);
                oppCount = opponent->getCardCount(CardType::COMMERCIAL);
                count = std::max(selfCount, oppCount);
                break;
            case GuildCriteria::BLUE_CARDS:
                selfCount = self->getCardCount(CardType::CIVILIAN);
                oppCount = opponent->getCardCount(CardType::CIVILIAN);
                count = std::max(selfCount, oppCount);
                break;
            case GuildCriteria::GREEN_CARDS:
                selfCount = self->getCardCount(CardType::SCIENTIFIC);
                oppCount = opponent->getCardCount(CardType::SCIENTIFIC);
                count = std::max(selfCount, oppCount);
                break;
            case GuildCriteria::RED_CARDS:
                selfCount = self->getCardCount(CardType::MILITARY);
                oppCount = opponent->getCardCount(CardType::MILITARY);
                count = std::max(selfCount, oppCount);
                break;
            case GuildCriteria::BROWN_GREY_CARDS: // Shipowners
                selfCount = self->getCardCount(CardType::RAW_MATERIAL) + self->getCardCount(CardType::MANUFACTURED);
                oppCount = opponent->getCardCount(CardType::RAW_MATERIAL) + opponent->getCardCount(CardType::MANUFACTURED);
                count = std::max(selfCount, oppCount);
                break;
            case GuildCriteria::WONDERS: // Builders - No Coins
                multiplier = 0;
                break;
            case GuildCriteria::COINS: // Moneylenders - No Coins
                multiplier = 0;
                break;
            default: break;
        }

        if (count > 0 && multiplier > 0) {
            int total = count * multiplier;
            self->gainCoins(total);
        }
    }

    int GuildEffect::calculateScore(const Player* self, const Player* opponent) const {
        int count = 0;
        int selfCount = 0;
        int oppCount = 0;

        switch (criteria) {
            case GuildCriteria::YELLOW_CARDS:
                selfCount = self->getCardCount(CardType::COMMERCIAL);
                oppCount = opponent->getCardCount(CardType::COMMERCIAL);
                count = std::max(selfCount, oppCount);
                return count * 1;
            case GuildCriteria::BLUE_CARDS:
                selfCount = self->getCardCount(CardType::CIVILIAN);
                oppCount = opponent->getCardCount(CardType::CIVILIAN);
                count = std::max(selfCount, oppCount);
                return count * 1;
            case GuildCriteria::GREEN_CARDS:
                selfCount = self->getCardCount(CardType::SCIENTIFIC);
                oppCount = opponent->getCardCount(CardType::SCIENTIFIC);
                count = std::max(selfCount, oppCount);
                return count * 1;
            case GuildCriteria::RED_CARDS:
                selfCount = self->getCardCount(CardType::MILITARY);
                oppCount = opponent->getCardCount(CardType::MILITARY);
                count = std::max(selfCount, oppCount);
                return count * 1;
            case GuildCriteria::BROWN_GREY_CARDS:
                selfCount = self->getCardCount(CardType::RAW_MATERIAL) + self->getCardCount(CardType::MANUFACTURED);
                oppCount = opponent->getCardCount(CardType::RAW_MATERIAL) + opponent->getCardCount(CardType::MANUFACTURED);
                count = std::max(selfCount, oppCount);
                return count * 1;
            case GuildCriteria::WONDERS:
                 selfCount = self->builtWonders.size();
                 oppCount = opponent->builtWonders.size();
                 count = std::max(selfCount, oppCount);
                 return count * 2; // Builders Guild: 2 VP per Wonder
            case GuildCriteria::COINS:
                 selfCount = self->coins;
                 oppCount = opponent->coins;
                 count = std::max(selfCount, oppCount);
                 return count / 3; // Moneylenders: 1 VP per 3 Coins set
            default: return 0;
        }
    }

    std::string GuildEffect::getDescription() const { return "Guild Effect"; }
}