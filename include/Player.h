//
// Created by choyichi on 2025/12/15.
//

#ifndef SEVEN_WONDERS_DUEL_PLAYER_H
#define SEVEN_WONDERS_DUEL_PLAYER_H

#include "Global.h"
#include "Card.h"
#include "EffectSystem.h" // 包含 Effect 定义
#include <vector>
#include <map>
#include <set>
#include <string>
#include <algorithm>

namespace SevenWondersDuel {

    class Player {
    private:
        // 基础属性
        int m_id; // 0 或 1
        std::string m_name;
        int m_coins;

        // 持有的卡牌与奇迹
        std::vector<Card*> m_builtCards;
        std::vector<Wonder*> m_builtWonders;    // 已建成的奇迹
        std::vector<Wonder*> m_unbuiltWonders;  // 轮抽拿到但未建的奇迹

        // 状态统计 (缓存以提高性能)
        std::map<ResourceType, int> m_fixedResources; // 玩家拥有的总固定资源 (用于建造)
        std::map<ResourceType, int> m_publicProduction; // 玩家对对手可见的公开资源 (用于增加对手交易费)

        std::vector<std::vector<ResourceType>> m_choiceResources; // 多选一资源 (如: 木/土/石)

        std::map<ScienceSymbol, int> m_scienceSymbols;
        // 记录已触发过"配对奖励"的符号，避免重复触发
        std::set<ScienceSymbol> m_claimedSciencePairs;

        std::set<std::string> m_ownedChainTags; // 拥有的连锁标记 (如 "MOON")

        std::set<ProgressToken> m_progressTokens;

        // 特殊Buff标志位
        std::map<ResourceType, bool> m_tradingDiscounts; // 对应黄色卡牌：石/木/土/纸/玻 交易仅需1金币

    public:
        // 构造函数
        Player(int pid, std::string pname);

        // --- Getters ---
        int getId() const { return m_id; }
        const std::string& getName() const { return m_name; }
        int getCoins() const { return m_coins; }

        const std::vector<Card*>& getBuiltCards() const { return m_builtCards; }
        const std::vector<Wonder*>& getBuiltWonders() const { return m_builtWonders; }
        const std::vector<Wonder*>& getUnbuiltWonders() const { return m_unbuiltWonders; }

        const std::map<ResourceType, int>& getFixedResources() const { return m_fixedResources; }
        const std::map<ResourceType, int>& getPublicProduction() const { return m_publicProduction; }
        const std::vector<std::vector<ResourceType>>& getChoiceResources() const { return m_choiceResources; }

        const std::map<ScienceSymbol, int>& getScienceSymbols() const { return m_scienceSymbols; }
        const std::set<ScienceSymbol>& getClaimedSciencePairs() const { return m_claimedSciencePairs; }
        
        const std::set<std::string>& getOwnedChainTags() const { return m_ownedChainTags; }
        const std::set<ProgressToken>& getProgressTokens() const { return m_progressTokens; }
        const std::map<ResourceType, bool>& getTradingDiscounts() const { return m_tradingDiscounts; }

        // --- 核心状态查询 ---

        // 获取某类卡牌的数量 (用于行会卡计分)
        int getCardCount(CardType type) const;

        // --- 资源与购买逻辑 ---

        // 向银行购买资源的单价
        int getTradingPrice(ResourceType type, const Player& opponent) const;

        // 检查是否能负担费用，并返回实际开销
        // [UPDATED] 增加 CardType 参数以支持 Masonry/Architecture 等科技标记减费
        std::pair<bool, int> calculateCost(const ResourceCost& cost, const Player& opponent, CardType targetType) const;

        // --- 动作执行 (Mutators) ---

        void payCoins(int amount);
        void gainCoins(int amount);
        
        // [Refactor] 新增：用于 EffectSystem 等
        void setTradingDiscount(ResourceType r, bool active);
        void addClaimedSciencePair(ScienceSymbol s);

        // 接收各种资源/Buff
        void addResource(ResourceType type, int count, bool isTradable);
        void addProductionChoice(const std::vector<ResourceType>& choices);
        void addScienceSymbol(ScienceSymbol s);
        void addChainTag(const std::string& tag);
        void addProgressToken(ProgressToken token);

        // 建造卡牌
        void constructCard(Card* card);

        // 移除卡牌 (用于被摧毁) - 返回被移除的卡牌指针，若无则 nullptr
        Card* removeCardByType(CardType type);

        // 奇迹管理
        void addUnbuiltWonder(Wonder* w);
        void removeUnbuiltWonder(const std::string& wonderId);
        void clearUnbuiltWonders();
        
        // 建造奇迹
        void constructWonder(std::string wonderId, Card* overlayCard);
    };
}

#endif // SEVEN_WONDERS_DUEL_PLAYER_H/