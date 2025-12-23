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
    struct ResourceCost {
        int coins = 0;
        std::map<ResourceType, int> resources;

        bool isFree() const { return coins == 0 && resources.empty(); }
    };

    // 卡牌定义
    class Card {
        friend class DataLoader;
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

        // 辅助方法：获取卡牌的基础分 (直接的分 + 效果计算的分)
        // 注意：这里只计算直接写在卡面上的分，行会卡等动态分需要传入 Context，
        // 暂时只提供简单接口，复杂计算在 Player 类中聚合。
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
        friend class DataLoader;
        friend class Player; // For constructing wonder (state change)
        // Or better: provide a public method to mutate state
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

    // 用于金字塔布局的节点
    // CardSlot keeps public members as it functions more like a struct used internally by Board/Pyramid
    // However, we should use getters for card access if we want consistency.
    struct CardSlot {
        std::string id;      // 对应 Card 的 ID (Cache)
        Card* cardPtr = nullptr;
        bool isFaceUp = false;
        bool isRemoved = false;

        int row = 0;
        int index = 0; // 行内索引

        // 图结构依赖
        std::vector<int> coveredBy; // 压着我的牌的 Slot 索引
    };

}

#endif // SEVEN_WONDERS_DUEL_CARD_H
