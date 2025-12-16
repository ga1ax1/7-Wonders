//
// Created by choyichi on 2025/12/15.
//

#include "Player.h"
#include <limits>
#include <numeric>

namespace SevenWondersDuel {

    // 辅助：递归求解最小交易成本
    // needed: 当前仍缺少的资源
    // choiceIdx: 当前处理到第几个"多选一"资源
    // choices: 玩家拥有的所有多选一能力
    // opponent: 对手引用 (用于查询交易价格)
    // parent: 玩家指针 (用于查询是否有交易优惠)
    void solveMinCost(std::map<ResourceType, int> needed,
                      size_t choiceIdx,
                      const std::vector<std::vector<ResourceType>>& choices,
                      const Player& opponent,
                      const Player& parent,
                      int& minCost)
    {
        // 剪枝：如果当前路径的已知成本已经超过当前发现的最小值，没必要继续
        // (简单场景下暂略，直接算到底)

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

        // 优化：如果这个多选一里的选项都不是我需要的，随便选一个往下走即可
        // 或者简单地遍历所有选项
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

        // 如果提供的选项都没用 (比如产木/土，但我缺石)，那就都不选(或者随便选一个)，直接看下一个
        if (!usefulOptionFound) {
            solveMinCost(needed, choiceIdx + 1, choices, opponent, parent, minCost);
        }
    }

    std::pair<bool, int> Player::calculateCost(const ResourceCost& cost, const Player& opponent) const {
        // 1. 基础金币检查
        if (coins < cost.coins) {
            // 连基础造价的钱都不够
             // 这里返回的 cost 是缺口，虽然实际上直接 fail
            return { false, cost.coins };
        }

        // 2. 准备计算资源缺口
        std::map<ResourceType, int> deficit = cost.resources;

        // 3. 扣除固定产出
        for (auto it = deficit.begin(); it != deficit.end(); ) {
            ResourceType type = it->first;
            int needed = it->second;

            // 查找我拥有的固定资源
            auto myResIt = fixedResources.find(type);
            int owned = (myResIt != fixedResources.end()) ? myResIt->second : 0;

            if (owned >= needed) {
                // 够了，不需要了
                it = deficit.erase(it);
            } else {
                // 不够，减去拥有的
                it->second -= owned;
                ++it;
            }
        }

        // 如果扣除固定产出后没缺口了，且金币够
        if (deficit.empty()) {
            return { true, cost.coins };
        }

        // 4. 利用多选一资源填补缺口 (寻找最小交易费)
        int minTradingCost = std::numeric_limits<int>::max();

        // 调用递归求解
        // 注意：这里传入的是值拷贝的 deficit，因为递归中会修改
        // const_cast 是为了在 const 函数中调用 helper，或者 helper 设为 static/friend
        // 这里直接传 *this 即可
        solveMinCost(deficit, 0, choiceResources, opponent, *this, minTradingCost);

        // 5. 汇总结果
        int totalRequired = cost.coins + minTradingCost;
        bool canAfford = (coins >= totalRequired);

        return { canAfford, totalRequired };
    }

}