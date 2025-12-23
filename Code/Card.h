//
// Created by choyichi on 2025/12/15.
//

#ifndef SEVEN_WONDERS_DUEL_CARD_H
#define SEVEN_WONDERS_DUEL_CARD_H

#include "Global.h"
#include "EffectSystem.h"
#include <string>
#include <vector>
#include <memory>
#include <map>

namespace SevenWondersDuel {

    class DataLoader; // Forward declaration

    // 统一费用结构
    class ResourceCost {
    private:
        int m_coins = 0;
        std::map<ResourceType, int> m_resources;

    public:
        ResourceCost() = default;
        
        // Getters
        int getCoins() const { return m_coins; }
        const std::map<ResourceType, int>& getResources() const { return m_resources; }
        
        // Setters
        void setCoins(int coins) { m_coins = coins; }
        void setResources(const std::map<ResourceType, int>& res) { m_resources = res; }
        void addResource(ResourceType type, int count) { m_resources[type] += count; }

        bool isFree() const { return m_coins == 0 && m_resources.empty(); }
    };

    // 前向声明
    class Card;
    class CardPyramid;
    class Board;
    class GameController;
    class GameView;

    // 用于金字塔布局的节点
    class CardSlot {
    private:
        std::string m_id;      // 对应 Card 的 ID (Cache)
        Card* m_cardPtr = nullptr;
        bool m_isFaceUp = false;
        bool m_isRemoved = false;

        int m_row = 0;
        int m_index = 0; // 行内索引

        // 图结构依赖
        std::vector<int> m_coveredBy; // 压着我的牌的 Slot 索引

    public:
        CardSlot() = default;

        // Getters
        const std::string& getId() const { return m_id; }
        Card* getCardPtr() const { return m_cardPtr; }
        bool isFaceUp() const { return m_isFaceUp; }
        bool isRemoved() const { return m_isRemoved; }
        int getRow() const { return m_row; }
        int getIndex() const { return m_index; }
        const std::vector<int>& getCoveredBy() const { return m_coveredBy; }

        // Mutators
        void setId(const std::string& id) { m_id = id; }
        void setCardPtr(Card* ptr) { m_cardPtr = ptr; }
        void setFaceUp(bool val) { m_isFaceUp = val; }
        void setRemoved(bool val) { m_isRemoved = val; }
        void setRow(int r) { m_row = r; }
        void setIndex(int i) { m_index = i; }
        
        // 图操作
        void addCoveredBy(int index) { m_coveredBy.push_back(index); }
        
        // Notify that a covering card was removed. Returns true if this card just became fully uncovered.
        bool notifyCoveringRemoved(int index) {
            auto it = std::remove(m_coveredBy.begin(), m_coveredBy.end(), index);
            if (it != m_coveredBy.end()) {
                m_coveredBy.erase(it, m_coveredBy.end());
                if (m_coveredBy.empty()) {
                    if (!m_isFaceUp) {
                        m_isFaceUp = true;
                        return true;
                    }
                }
            }
            return false;
        }
    };

    // 卡牌定义
    class Card {
    private:
        std::string m_id;
        std::string m_name;
        int m_age = 0; // 1, 2, 3
        CardType m_type = CardType::CIVILIAN;

        ResourceCost m_cost;

        // 连锁机制
        std::string m_chainTag;          // 此卡提供的标记 (如 "MOON")
        std::string m_requiresChainTag;  // 此卡需要的标记 (如 "MOON" -> 免费)

        // 效果列表 (多态)
        std::vector<std::shared_ptr<IEffect>> m_effects;

    public:
        // 构造函数
        Card() = default;

        // Getters
        const std::string& getId() const { return m_id; }
        const std::string& getName() const { return m_name; }
        int getAge() const { return m_age; }
        CardType getType() const { return m_type; }
        const ResourceCost& getCost() const { return m_cost; }
        const std::string& getChainTag() const { return m_chainTag; }
        const std::string& getRequiresChainTag() const { return m_requiresChainTag; }
        const std::vector<std::shared_ptr<IEffect>>& getEffects() const { return m_effects; }

        // Setters (DataLoader use)
        void setId(const std::string& id) { m_id = id; }
        void setName(const std::string& name) { m_name = name; }
        void setAge(int age) { m_age = age; }
        void setType(CardType type) { m_type = type; }
        void setCost(const ResourceCost& cost) { m_cost = cost; }
        void setChainTag(const std::string& tag) { m_chainTag = tag; }
        void setRequiresChainTag(const std::string& tag) { m_requiresChainTag = tag; }
        void setEffects(std::vector<std::shared_ptr<IEffect>> effects) { m_effects = std::move(effects); }

        // 辅助方法：获取卡牌的基础分 (直接的分 + 效果计算的分)
        int getVictoryPoints(const Player* self, const Player* opponent) const {
            int total = 0;
            for(const auto& eff : m_effects) {
                total += eff->calculateScore(self, opponent);
            }
            return total;
        }
    };

    // 奇迹定义
    class Wonder {
    private:
        std::string m_id;
        std::string m_name;
        ResourceCost m_cost;

        std::vector<std::shared_ptr<IEffect>> m_effects;

        // 状态
        bool m_isBuilt = false;
        // 建造奇迹时垫在下面的那张牌 (用于查看或者甚至可能被某些能力回收)
        const Card* m_builtOverlayCard = nullptr;

    public:
        Wonder() = default;

        // Getters
        const std::string& getId() const { return m_id; }
        const std::string& getName() const { return m_name; }
        const ResourceCost& getCost() const { return m_cost; }
        const std::vector<std::shared_ptr<IEffect>>& getEffects() const { return m_effects; }
        bool isBuilt() const { return m_isBuilt; }
        const Card* getBuiltOverlayCard() const { return m_builtOverlayCard; }

        // Setters (DataLoader use)
        void setId(const std::string& id) { m_id = id; }
        void setName(const std::string& name) { m_name = name; }
        void setCost(const ResourceCost& cost) { m_cost = cost; }
        void setEffects(std::vector<std::shared_ptr<IEffect>> effects) { m_effects = std::move(effects); }

        // Mutators
        void build(const Card* overlay) {
            m_isBuilt = true;
            m_builtOverlayCard = overlay;
        }
        
        // Reset state (e.g. for new game)
        void reset() {
            m_isBuilt = false;
            m_builtOverlayCard = nullptr;
        }

        int getVictoryPoints(const Player* self, const Player* opponent) const {
            if (!m_isBuilt) return 0;
            int total = 0;
            for(const auto& eff : m_effects) {
                total += eff->calculateScore(self, opponent);
            }
            return total;
        }
    };

}

#endif // SEVEN_WONDERS_DUEL_CARD_H