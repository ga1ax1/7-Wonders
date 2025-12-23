//
// Created by choyichi on 2025/12/15.
//

#include "EffectSystem.h"
#include "Player.h"
#include <algorithm>
#include <sstream>

namespace SevenWondersDuel {

    // --- 1. ProductionEffect ---
    void ProductionEffect::apply(Player* self, Player* opponent, IEffectContext* ctx) {
        if (isChoice) {
            std::vector<ResourceType> choices;
            for (auto const& [type, count] : producedResources) {
                choices.push_back(type);
            }
            self->addProductionChoice(choices);
        } else {
            for (auto const& [type, count] : producedResources) {
                self->addResource(type, count, isTradable);
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
    void MilitaryEffect::apply(Player* self, Player* opponent, IEffectContext* ctx) {
        int finalShields = shields;

        // 规则修复：Strategy Token 仅对军事建筑 (Red Cards) 生效，+1 盾
        if (isFromCard && self->getProgressTokens().count(ProgressToken::STRATEGY)) {
            finalShields += 1;
            ctx->addLog("[Effect] Strategy Token adds +1 Shield.");
        }

        auto lootEvents = ctx->moveMilitary(finalShields, self->getId());

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
    void ScienceEffect::apply(Player* self, Player* opponent, IEffectContext* ctx) {
        self->addScienceSymbol(symbol);
        // 配对逻辑已在 GameController::handleBuildCard 中通过 checkForNewSciencePairs 统一处理
    }

    std::string ScienceEffect::getDescription() const {
        return "Science Symbol";
    }

    // --- 4. VictoryPointEffect ---
    void VictoryPointEffect::apply(Player* self, Player* opponent, IEffectContext* ctx) {
        // 立即效果无，只计分
    }

    int VictoryPointEffect::calculateScore(const Player* self, const Player* opponent) const {
        return points;
    }

    std::string VictoryPointEffect::getDescription() const {
        return "VP: " + std::to_string(points);
    }

    // --- 5. CoinEffect ---
    void CoinEffect::apply(Player* self, Player* opponent, IEffectContext* ctx) {
        self->gainCoins(amount);
    }

    std::string CoinEffect::getDescription() const {
        return "Coins: " + std::to_string(amount);
    }

    // --- 6. CoinsPerTypeEffect (商业/行会) ---
    void CoinsPerTypeEffect::apply(Player* self, Player* opponent, IEffectContext* ctx) {
        int count = 0;
        count += self->getCardCount(targetType);

        if (countWonder) {
            count += self->getBuiltWonders().size();
        }

        int bonus = count * coinsPerCard;
        self->gainCoins(bonus);
    }

    std::string CoinsPerTypeEffect::getDescription() const {
        return "Coins per card type: " + std::to_string(coinsPerCard);
    }

    // --- 7. TradeDiscountEffect ---
    void TradeDiscountEffect::apply(Player* self, Player* opponent, IEffectContext* ctx) {
        self->setTradingDiscount(resource, true);
    }

    std::string TradeDiscountEffect::getDescription() const {
        return "Fixed trading price (1 coin) for " + resourceToString(resource);
    }

    // --- 8. DestroyCardEffect ---
    void DestroyCardEffect::apply(Player* self, Player* opponent, IEffectContext* ctx) {
        ctx->setPendingDestructionType(targetColor);
        ctx->setState(GameState::WAITING_FOR_DESTRUCTION);
    }

    std::string DestroyCardEffect::getDescription() const {
        return "Destroy an opponent's card of specific color.";
    }

    // --- 9. ExtraTurnEffect ---
    void ExtraTurnEffect::apply(Player* self, Player* opponent, IEffectContext* ctx) {
        ctx->grantExtraTurn();
    }

    // --- 10. BuildFromDiscardEffect ---
    void BuildFromDiscardEffect::apply(Player* self, Player* opponent, IEffectContext* ctx) {
        // [UPDATED] 如果弃牌堆为空，则不触发等待状态，直接记录日志
        if (ctx->isDiscardPileEmpty()) {
             ctx->addLog("[Effect] Discard pile is empty. Mausoleum effect skipped.");
             return;
        }
        ctx->setState(GameState::WAITING_FOR_DISCARD_BUILD);
    }

    // --- 11. ProgressTokenSelectEffect ---
    void ProgressTokenSelectEffect::apply(Player* self, Player* opponent, IEffectContext* ctx) {
        ctx->setState(GameState::WAITING_FOR_TOKEN_SELECTION_LIB);
    }

    // --- 12. OpponentLoseCoinsEffect ---
    void OpponentLoseCoinsEffect::apply(Player* self, Player* opponent, IEffectContext* ctx) {
        int loss = std::min(opponent->getCoins(), amount);
        opponent->payCoins(loss);
    }
    std::string OpponentLoseCoinsEffect::getDescription() const {
        return "Opponent loses " + std::to_string(amount) + " coins.";
    }

    // --- 13. GuildEffect Strategies ---

    class CommercialGuildStrategy : public IGuildStrategy {
    public:
        int calculateCoins(const Player* self, const Player* opponent) const override {
            return std::max(self->getCardCount(CardType::COMMERCIAL), opponent->getCardCount(CardType::COMMERCIAL));
        }
        int calculateVP(const Player* self, const Player* opponent) const override {
            return calculateCoins(self, opponent);
        }
    };

    class ProductionGuildStrategy : public IGuildStrategy {
    public:
        int calculateCoins(const Player* self, const Player* opponent) const override {
            int sCount = self->getCardCount(CardType::RAW_MATERIAL) + self->getCardCount(CardType::MANUFACTURED);
            int oCount = opponent->getCardCount(CardType::RAW_MATERIAL) + opponent->getCardCount(CardType::MANUFACTURED);
            return std::max(sCount, oCount);
        }
        int calculateVP(const Player* self, const Player* opponent) const override {
            return calculateCoins(self, opponent);
        }
    };

    class BuildersGuildStrategy : public IGuildStrategy {
    public:
        int calculateCoins(const Player* self, const Player* opponent) const override { return 0; }
        int calculateVP(const Player* self, const Player* opponent) const override {
            return std::max((int)self->getBuiltWonders().size(), (int)opponent->getBuiltWonders().size()) * 2;
        }
    };

    class CivilianGuildStrategy : public IGuildStrategy {
    public:
        int calculateCoins(const Player* self, const Player* opponent) const override {
            return std::max(self->getCardCount(CardType::CIVILIAN), opponent->getCardCount(CardType::CIVILIAN));
        }
        int calculateVP(const Player* self, const Player* opponent) const override {
            return calculateCoins(self, opponent);
        }
    };

    class ScientificGuildStrategy : public IGuildStrategy {
    public:
        int calculateCoins(const Player* self, const Player* opponent) const override {
            return std::max(self->getCardCount(CardType::SCIENTIFIC), opponent->getCardCount(CardType::SCIENTIFIC));
        }
        int calculateVP(const Player* self, const Player* opponent) const override {
            return calculateCoins(self, opponent);
        }
    };

    class MilitaryGuildStrategy : public IGuildStrategy {
    public:
        int calculateCoins(const Player* self, const Player* opponent) const override {
            return std::max(self->getCardCount(CardType::MILITARY), opponent->getCardCount(CardType::MILITARY));
        }
        int calculateVP(const Player* self, const Player* opponent) const override {
            return calculateCoins(self, opponent);
        }
    };

    class MoneylendersGuildStrategy : public IGuildStrategy {
    public:
        int calculateCoins(const Player* self, const Player* opponent) const override { return 0; }
        int calculateVP(const Player* self, const Player* opponent) const override {
            return std::max(self->getCoins(), opponent->getCoins()) / 3;
        }
    };

    GuildEffect::GuildEffect(GuildCriteria c) {
        switch (c) {
            case GuildCriteria::YELLOW_CARDS: m_strategy = std::make_unique<CommercialGuildStrategy>(); break;
            case GuildCriteria::BROWN_GREY_CARDS: m_strategy = std::make_unique<ProductionGuildStrategy>(); break;
            case GuildCriteria::WONDERS: m_strategy = std::make_unique<BuildersGuildStrategy>(); break;
            case GuildCriteria::BLUE_CARDS: m_strategy = std::make_unique<CivilianGuildStrategy>(); break;
            case GuildCriteria::GREEN_CARDS: m_strategy = std::make_unique<ScientificGuildStrategy>(); break;
            case GuildCriteria::RED_CARDS: m_strategy = std::make_unique<MilitaryGuildStrategy>(); break;
            case GuildCriteria::COINS: m_strategy = std::make_unique<MoneylendersGuildStrategy>(); break;
        }
    }

    void GuildEffect::apply(Player* self, Player* opponent, IEffectContext* ctx) {
        int coins = m_strategy->calculateCoins(self, opponent);
        if (coins > 0) self->gainCoins(coins);
    }

    int GuildEffect::calculateScore(const Player* self, const Player* opponent) const {
        return m_strategy->calculateVP(self, opponent);
    }

    std::string GuildEffect::getDescription() const { return "Guild Effect"; }
}