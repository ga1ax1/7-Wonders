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

    // 统一费用结构
    struct ResourceCost {
        int coins = 0;
        std::map<ResourceType, int> resources;

        bool isFree() const { return coins == 0 && resources.empty(); }
    };

    // 卡牌定义
    struct Card {
        std::string id;
        std::string name;
        int age; // 1, 2, 3
        CardType type;

        ResourceCost cost;

        // 连锁机制
        std::string chainTag;          // 此卡提供的标记 (如 "MOON")
        std::string requiresChainTag;  // 此卡需要的标记 (如 "MOON" -> 免费)

        // 效果列表 (多态)
        std::vector<std::shared_ptr<IEffect>> effects;

        // 构造函数
        Card() = default;

        // 辅助方法：获取卡牌的基础分 (直接的分 + 效果计算的分)
        // 注意：这里只计算直接写在卡面上的分，行会卡等动态分需要传入 Context，
        // 暂时只提供简单接口，复杂计算在 Player 类中聚合。
        int getVictoryPoints(const Player* self, const Player* opponent) const {
            int total = 0;
            for(const auto& eff : effects) {
                total += eff->calculateScore(self, opponent);
            }
            return total;
        }
    };

    // 奇迹定义
    struct Wonder {
        std::string id;
        std::string name;
        ResourceCost cost;

        std::vector<std::shared_ptr<IEffect>> effects;

        // 状态
        bool isBuilt = false;
        // 建造奇迹时垫在下面的那张牌 (用于查看或者甚至可能被某些能力回收)
        const Card* builtOverlayCard = nullptr;

        int getVictoryPoints(const Player* self, const Player* opponent) const {
            if (!isBuilt) return 0;
            int total = 0;
            for(const auto& eff : effects) {
                total += eff->calculateScore(self, opponent);
            }
            return total;
        }
    };

    // 用于金字塔布局的节点
    struct CardSlot {
        std::string id;      // 对应 Card 的 ID
        Card* cardPtr = nullptr;
        bool isFaceUp = false;
        bool isRemoved = false;

        int row;
        int index; // 行内索引

        // 图结构依赖
        std::vector<int> coveredBy; // 压着我的牌的 Slot 索引
    };

}

#endif // SEVEN_WONDERS_DUEL_CARD_H