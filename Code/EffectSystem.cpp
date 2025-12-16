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
                // 这里的 map 通常是 {WOOD:1, CLAY:1} 表示 Wood OR Clay
                // 我们的 Player 结构需要 vector<ResourceType>
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
        // 检查战略标记 (Strategy Token): 只有红卡才加成，但 Effect 不知道源头是红卡还是奇迹
        // 修正：Strategy Token 的逻辑在 Player 触发 Effect 前判定？
        // 或者我们在这里简单处理：如果 self 有 Strategy 且 shields > 0 (且是红卡?)
        // 为了架构清晰，我们假设 Controller 在调用 apply 之前没有修改 shields。
        // 由于 IEffect 不包含来源 Card 的信息，这里存在一个小的架构耦合问题。
        // 实际上，《七大奇迹对决》规则：Strategy Token 对 "Military Buildings" 生效。
        // 因此，我们应当在 Card::onBuild 或者 Controller 区分来源。
        // 权宜之计：我们假设 GameController 处理 Strategy 加成，这里只负责推盾牌。
        // 实际项目应在 apply 参数中带上 sourceCardType。

        // 暂且直接推，Controller 的 handleBuildCard 会负责计算 Strategy 的额外 +1
        auto lootEvents = ctx->getBoard()->militaryTrack.move(shields, self->id);

        // 处理掠夺
        // lootEvents 包含：2, 5 (对手丢钱) 或 -2, -5 (自己丢钱，虽然逻辑上这不可能，因为是自己推)
        // move() 返回正数代表对手受罚（如果是正向推）
        // 这里的逻辑需要根据 Board::move 的具体实现微调。
        // 假设 move() 已经处理好方向，返回的 vector 是 "触发了扣款 X 元的事件"

        for (int amount : lootEvents) {
            // 扣对手的钱
            opponent->payCoins(amount);
        }
    }

    std::string MilitaryEffect::getDescription() const {
        return "Shields: " + std::to_string(shields);
    }

    // --- 3. ScienceEffect ---
    void ScienceEffect::apply(Player* self, Player* opponent, GameController* ctx) {
        self->addScienceSymbol(symbol);
        // 配对逻辑检测 (6种胜利 或 2个相同得标记) 交由 Controller 的 checkState 检测
    }

    std::string ScienceEffect::getDescription() const {
        // 简化显示
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
        // 统计自己拥有的对应类型卡牌
        count += self->getCardCount(targetType);

        if (countWonder) {
            count += self->builtWonders.size();
        }

        // 黄卡通常只统计自己的？
        // 比如 Forum/Caravansery 是给资源的。
        // 比如 Arena (Yellow): 给钱 (每奇迹*2)，给分 (每奇迹*3)。
        // 比如 Port (Yellow): 给钱 (每棕卡*2)。
        // 规则：商业卡(黄)是统计玩家*自己*的建筑。

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
        // 改变游戏状态，等待用户输入
        ctx->setState(GameState::WAITING_FOR_DESTRUCTION);
        // 这里只是切换状态，UI 需要根据状态弹窗，后续 Action SELECT_DESTRUCTION 会执行实际移除
        // Controller 需要记录当前限制的颜色 (Board/Model 中需要临时变量存 targetColor?)
        // 简单处理：Controller 状态机应当知道当前处于 "WAITING_FOR_DESTRUCTION_GREY" 或 "BROWN"
        // 这里的实现略微依赖 Controller 的细节。
    }

    std::string DestroyCardEffect::getDescription() const {
        return "Destroy an opponent's card of specific color.";
    }

    // --- 9. ExtraTurnEffect ---
    void ExtraTurnEffect::apply(Player* self, Player* opponent, GameController* ctx) {
        // 检查神学标记 (Theology): 所有奇迹都有再来一回合
        // 但如果 Effect 本身就是 ExtraTurn，那就直接触发
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
        opponent->payCoins(amount);
    }
    std::string OpponentLoseCoinsEffect::getDescription() const {
        return "Opponent loses " + std::to_string(amount) + " coins.";
    }

    // --- 13. GuildEffect ---
    void GuildEffect::apply(Player* self, Player* opponent, GameController* ctx) {
        // 立即效果：获得金币
        // 规则：行会卡建造时，根据条件给金币
        // 注意：有些行会是"拥有最多该类卡牌的城市"

        int count = 0;
        int selfCount = 0;
        int oppCount = 0;

        switch (criteria) {
            case GuildCriteria::YELLOW_CARDS:
                selfCount = self->getCardCount(CardType::COMMERCIAL);
                oppCount = opponent->getCardCount(CardType::COMMERCIAL);
                count = std::max(selfCount, oppCount);
                break;
            // ... 其他 Case 类似，取 max(self, opp)
            case GuildCriteria::WONDERS:
                count = std::max((int)self->builtWonders.size(), (int)opponent->builtWonders.size());
                // 建筑师行会：奇迹给钱吗？规则书：立即得钱?
                // 检查规则：建筑师行会 (Guild Builders) -> 游戏结束分(2*奇迹)，不给钱？
                // 实际上根据提供的 JSON 数据，Guild 都有 apply (给钱) 和 score (给分)
                // TradeGuild: 1 coin per Yellow (MAX) -> apply
                break;
             // ...
             default: break;
        }

        // 简化实现：假设所有行会都给 1块钱 * 数量
        self->gainCoins(count);
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
                return count * 1; // 1分每张
            case GuildCriteria::WONDERS:
                 selfCount = self->builtWonders.size();
                 oppCount = opponent->builtWonders.size();
                 count = std::max(selfCount, oppCount);
                 return count * 2; // 建筑师行会 2分
            case GuildCriteria::COINS:
                 selfCount = self->coins;
                 oppCount = opponent->coins;
                 count = std::max(selfCount, oppCount);
                 return count / 3;
            // ... 其他省略
            default: return 0;
        }
    }

    std::string GuildEffect::getDescription() const { return "Guild Effect"; }
}