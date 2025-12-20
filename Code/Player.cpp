//
// Created by choyichi on 2025/12/15.
//

#include "Player.h"
#include <limits>
#include <numeric>
#include <vector>
#include <algorithm>
#include <map>

namespace SevenWondersDuel {

    // 辅助：递归求解最小交易成本
    void solveMinCost(std::map<ResourceType, int> needed,
                      size_t choiceIdx,
                      const std::vector<std::vector<ResourceType>>& choices,
                      const Player& opponent,
                      const Player& parent,
                      int& minCost)
    {
        // 基准情况：所有多选一资源都分配完毕
        if (choiceIdx == choices.size()) {
            int currentTradingCost = 0;
            for (auto const& [type, count] : needed) {
                if (count > 0) {
                    int unitPrice = parent.getTradingPrice(type, opponent);
                    currentTradingCost += count * unitPrice;
                }
            }
            if (currentTradingCost < minCost) {
                minCost = currentTradingCost;
            }
            return;
        }

        // 递归步骤：尝试当前多选一资源的每种可能性
        const auto& options = choices[choiceIdx];
        bool usefulOptionFound = false;

        for (ResourceType res : options) {
            // 如果这个资源是我需要的，这是一种有意义的分支
            if (needed[res] > 0) {
                usefulOptionFound = true;
                std::map<ResourceType, int> nextNeeded = needed;
                nextNeeded[res]--;
                solveMinCost(nextNeeded, choiceIdx + 1, choices, opponent, parent, minCost);
            }
        }

        // 如果提供的选项都没用，那就都不选，直接看下一个
        if (!usefulOptionFound) {
            solveMinCost(needed, choiceIdx + 1, choices, opponent, parent, minCost);
        }
    }

    int Player::getTradingPrice(ResourceType type, const Player& opponent) const {
        // 如果有特定资源的优惠卡 (如 Stone Reserve)，价格固定为 1
        if (tradingDiscounts.count(type) && tradingDiscounts.at(type)) return 1;

        // 否则：2 + 对手该类资源产量的"公开值" (棕/灰卡)
        int opponentProduction = 0;
        if (opponent.publicProduction.count(type)) {
            opponentProduction = opponent.publicProduction.at(type);
        }
        return 2 + opponentProduction;
    }

    // [UPDATED] 实现 Masonry 和 Architecture 的减费逻辑
    std::pair<bool, int> Player::calculateCost(const ResourceCost& cost, const Player& opponent, CardType targetType) const {
        // 1. 基础金币检查 (如果只需要金币)
        if (cost.resources.empty()) {
            if (coins < cost.coins) return { false, cost.coins };
            return { true, cost.coins };
        }

        // 2. 准备计算资源缺口
        std::map<ResourceType, int> deficit = cost.resources;

        // 3. 扣除固定产出
        for (auto it = deficit.begin(); it != deficit.end(); ) {
            ResourceType type = it->first;
            int needed = it->second;

            auto myResIt = fixedResources.find(type);
            int owned = (myResIt != fixedResources.end()) ? myResIt->second : 0;

            if (owned >= needed) {
                it = deficit.erase(it);
            } else {
                it->second -= owned;
                ++it;
            }
        }

        // --- [NEW] 科技标记减费逻辑 ---
        int discountCount = 0;
        if (progressTokens.count(ProgressToken::MASONRY) && targetType == CardType::CIVILIAN) {
            discountCount = 2;
        } else if (progressTokens.count(ProgressToken::ARCHITECTURE) && targetType == CardType::WONDER) {
            discountCount = 2;
        }

        // 智能减免：优先减免那些"如果不减免就很贵"的资源
        while (discountCount > 0 && !deficit.empty()) {
            // 寻找当前缺口中，交易单价最高的资源
            ResourceType bestToDiscount = ResourceType::WOOD;
            int maxPrice = -1;
            bool found = false;

            for (auto const& [type, count] : deficit) {
                if (count > 0) {
                    int price = getTradingPrice(type, opponent);
                    if (price > maxPrice) {
                        maxPrice = price;
                        bestToDiscount = type;
                        found = true;
                    }
                }
            }

            if (found) {
                deficit[bestToDiscount]--;
                if (deficit[bestToDiscount] <= 0) {
                    deficit.erase(bestToDiscount);
                }
                discountCount--;
            } else {
                break; // 没东西可减了
            }
        }
        // -----------------------------

        // 如果扣除固定产出和科技减免后没缺口了，且金币够
        if (deficit.empty()) {
            if (coins < cost.coins) return { false, cost.coins };
            return { true, cost.coins };
        }

        // 4. 利用多选一资源填补剩余缺口 (寻找最小交易费)
        int minTradingCost = std::numeric_limits<int>::max();

        solveMinCost(deficit, 0, choiceResources, opponent, *this, minTradingCost);

        // 5. 汇总结果
        int totalRequired = cost.coins + minTradingCost;
        bool canAfford = (coins >= totalRequired);

        return { canAfford, totalRequired };
    }

}