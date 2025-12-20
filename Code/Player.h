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
    public:
        // 基础属性
        int id; // 0 或 1
        std::string name;
        int coins;

        // 持有的卡牌与奇迹
        std::vector<Card*> builtCards;
        std::vector<Wonder*> builtWonders;    // 已建成的奇迹
        std::vector<Wonder*> unbuiltWonders;  // 轮抽拿到但未建的奇迹

        // 状态统计 (缓存以提高性能)
        std::map<ResourceType, int> fixedResources; // 玩家拥有的总固定资源 (用于建造)
        std::map<ResourceType, int> publicProduction; // 玩家对对手可见的公开资源 (用于增加对手交易费)

        std::vector<std::vector<ResourceType>> choiceResources; // 多选一资源 (如: 木/土/石)

        std::map<ScienceSymbol, int> scienceSymbols;
        // 记录已触发过"配对奖励"的符号，避免重复触发
        std::set<ScienceSymbol> claimedSciencePairs;

        std::set<std::string> ownedChainTags; // 拥有的连锁标记 (如 "MOON")

        std::set<ProgressToken> progressTokens;

        // 特殊Buff标志位
        std::map<ResourceType, bool> tradingDiscounts; // 对应黄色卡牌：石/木/土/纸/玻 交易仅需1金币

        // 构造函数
        Player(int pid, std::string pname) : id(pid), name(pname), coins(7) {
            // 初始化资源 map
            fixedResources[ResourceType::WOOD] = 0;
            fixedResources[ResourceType::STONE] = 0;
            fixedResources[ResourceType::CLAY] = 0;
            fixedResources[ResourceType::PAPER] = 0;
            fixedResources[ResourceType::GLASS] = 0;

            // 初始化公开产量
            publicProduction[ResourceType::WOOD] = 0;
            publicProduction[ResourceType::STONE] = 0;
            publicProduction[ResourceType::CLAY] = 0;
            publicProduction[ResourceType::PAPER] = 0;
            publicProduction[ResourceType::GLASS] = 0;

            // 交易优惠初始化
            tradingDiscounts[ResourceType::WOOD] = false;
            tradingDiscounts[ResourceType::STONE] = false;
            tradingDiscounts[ResourceType::CLAY] = false;
            tradingDiscounts[ResourceType::PAPER] = false;
            tradingDiscounts[ResourceType::GLASS] = false;
        }

        // --- 核心状态查询 ---

        // 获取某类卡牌的数量 (用于行会卡计分)
        int getCardCount(CardType type) const {
            int count = 0;
            for (const auto& card : builtCards) {
                if (card->type == type) count++;
            }
            return count;
        }

        // 计算当前总分 (胜利点数)
        int getScore(const Player& opponent) const {
            int score = 0;

            // 1. 卡牌分 + 效果分 (包含行会)
            for (const auto& card : builtCards) {
                score += card->getVictoryPoints(this, &opponent);
            }

            // 2. 奇迹分
            for (const auto& wonder : builtWonders) {
                score += wonder->getVictoryPoints(this, &opponent);
            }

            // 3. 军事分 (在 Board 中计算，此处不加)

            // 4. 金币分 (3块钱1分)
            score += coins / 3;

            // 5. 发展标记分
            for (auto token : progressTokens) {
                if (token == ProgressToken::AGRICULTURE) score += 4;
                if (token == ProgressToken::MATHEMATICS) score += 3 * progressTokens.size(); // 数学：每个标记3分
                if (token == ProgressToken::PHILOSOPHY) score += 7;
            }

            return score;
        }

        // 获取仅来自蓝色卡牌的分数 (用于平局判定)
        int getBlueCardScore(const Player& opponent) const {
            int score = 0;
            for (const auto& card : builtCards) {
                if (card->type == CardType::CIVILIAN) {
                    score += card->getVictoryPoints(this, &opponent);
                }
            }
            return score;
        }

        // --- 资源与购买逻辑 ---

        // 向银行购买资源的单价
        int getTradingPrice(ResourceType type, const Player& opponent) const;

        // 检查是否能负担费用，并返回实际开销
        // [UPDATED] 增加 CardType 参数以支持 Masonry/Architecture 等科技标记减费
        std::pair<bool, int> calculateCost(const ResourceCost& cost, const Player& opponent, CardType targetType) const;

        // --- 动作执行 ---

        void payCoins(int amount) {
            coins = std::max(0, coins - amount);
        }

        void gainCoins(int amount) {
            coins += amount;
        }

        // 接收各种资源/Buff
        void addResource(ResourceType type, int count, bool isTradable) {
            fixedResources[type] += count;
            if (isTradable) {
                publicProduction[type] += count;
            }
        }

        void addProductionChoice(const std::vector<ResourceType>& choices) {
            choiceResources.push_back(choices);
        }

        void addScienceSymbol(ScienceSymbol s) {
            if (s != ScienceSymbol::NONE) {
                scienceSymbols[s]++;
            }
        }

        void addChainTag(const std::string& tag) {
            if (!tag.empty()) ownedChainTags.insert(tag);
        }

        void addProgressToken(ProgressToken token) {
            progressTokens.insert(token);
            // 立即生效的 buff 处理 (如 LAW)
            if (token == ProgressToken::LAW) addScienceSymbol(ScienceSymbol::LAW);
        }

        // 建造卡牌
        void constructCard(Card* card) {
            builtCards.push_back(card);
            addChainTag(card->chainTag);
        }

        // 建造奇迹
        void constructWonder(std::string wonderId, Card* overlayCard) {
            auto it = std::find_if(unbuiltWonders.begin(), unbuiltWonders.end(),
                [&](Wonder* w){ return w->id == wonderId; });

            if (it != unbuiltWonders.end()) {
                Wonder* w = *it;
                w->isBuilt = true;
                w->builtOverlayCard = overlayCard;
                builtWonders.push_back(w);
                unbuiltWonders.erase(it);
            }
        }
    };
}

#endif // SEVEN_WONDERS_DUEL_PLAYER_H