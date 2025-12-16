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
        std::map<ResourceType, int> fixedResources; // 固定的资源 (如: 2木)
        std::vector<std::vector<ResourceType>> choiceResources; // 多选一资源 (如: 木/土/石)

        std::map<ScienceSymbol, int> scienceSymbols;
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
        // 注意：这不包含科技胜利或军事胜利的即时判定，只计算点数
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

            // 3. 军事分 (在 Board 中计算，此处不加，最终 Controller 会汇总)

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

        // --- 资源与购买逻辑 ---

        // 向银行购买资源的单价
        int getTradingPrice(ResourceType type, const Player& opponent) const {
            // 如果有特定资源的优惠卡 (如 Stone Reserve)，价格固定为 1
            if (tradingDiscounts.at(type)) return 1;

            // 否则：2 + 对手该类资源产量的"基础值"
            // 注意规则：只计算棕色/灰色卡牌提供的资源，奇迹/黄卡提供的多选一不算
            // 简化处理：我们要遍历对手的卡牌来看他有多少 FIXED 资源
            // (稍微优化的做法是 opponent 维护一个 public 的 fixedResources 计数器)
            int opponentProduction = opponent.fixedResources.at(type);
            return 2 + opponentProduction;
        }

        // 检查是否能负担费用，并返回实际开销
        // 返回值:
        //   pair.first: true=买得起, false=买不起
        //   pair.second: 需要支付给银行的总金币数 (包含交易费)
        // 注意：这是一个简化版的贪心/回溯算法需求，因为"多选一"资源需要最优匹配
        // 在这里我们只声明接口，具体实现在 .cpp
        std::pair<bool, int> calculateCost(const ResourceCost& cost, const Player& opponent) const;

        // --- 动作执行 ---

        void payCoins(int amount) {
            coins = std::max(0, coins - amount);
        }

        void gainCoins(int amount) {
            coins += amount;
        }

        // 接收各种资源/Buff
        void addResource(ResourceType type, int count) {
            fixedResources[type] += count;
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

        // 建造卡牌 (仅将卡牌加入列表并触发数据更新，扣钱逻辑在Controller)
        void constructCard(Card* card) {
            builtCards.push_back(card);
            // 这里不直接调 effects->apply，由 Controller 统一调度，
            // 否则会导致依赖循环或逻辑分散。
            // 但我们需要更新基础统计数据：
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