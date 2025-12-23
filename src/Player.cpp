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

    // 构造函数
    Player::Player(int pid, std::string pname) : m_id(pid), m_name(pname), m_coins(Config::INITIAL_COINS) {
        // 初始化资源 map
        m_fixedResources[ResourceType::WOOD] = 0;
        m_fixedResources[ResourceType::STONE] = 0;
        m_fixedResources[ResourceType::CLAY] = 0;
        m_fixedResources[ResourceType::PAPER] = 0;
        m_fixedResources[ResourceType::GLASS] = 0;

        // 初始化公开产量
        m_publicProduction[ResourceType::WOOD] = 0;
        m_publicProduction[ResourceType::STONE] = 0;
        m_publicProduction[ResourceType::CLAY] = 0;
        m_publicProduction[ResourceType::PAPER] = 0;
        m_publicProduction[ResourceType::GLASS] = 0;

        // 交易优惠初始化
        m_tradingDiscounts[ResourceType::WOOD] = false;
        m_tradingDiscounts[ResourceType::STONE] = false;
        m_tradingDiscounts[ResourceType::CLAY] = false;
        m_tradingDiscounts[ResourceType::PAPER] = false;
        m_tradingDiscounts[ResourceType::GLASS] = false;
    }

    // --- 核心状态查询 ---

    int Player::getCardCount(CardType type) const {
        int count = 0;
        for (const auto& card : m_builtCards) {
            if (card->getType() == type) count++;
        }
        return count;
    }

    // --- 辅助：递归求解最小交易成本 ---
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
        if (m_tradingDiscounts.count(type) && m_tradingDiscounts.at(type)) return 1;

        // 否则：2 + 对手该类资源产量的"公开值" (棕/灰卡)
        // Accessing private member of another instance of same class is allowed in C++
        int opponentProduction = 0;
        if (opponent.m_publicProduction.count(type)) {
            opponentProduction = opponent.m_publicProduction.at(type);
        }
        return Config::TRADING_BASE_COST + opponentProduction;
    }

    std::pair<bool, int> Player::calculateCost(const ResourceCost& cost, const Player& opponent, CardType targetType) const {
        // 1. 基础金币检查 (如果只需要金币)
        if (cost.getResources().empty()) {
            if (m_coins < cost.getCoins()) return { false, cost.getCoins() };
            return { true, cost.getCoins() };
        }

        // 2. 准备计算资源缺口
        std::map<ResourceType, int> deficit = cost.getResources();

        // 3. 扣除固定产出
        for (auto it = deficit.begin(); it != deficit.end(); ) {
            ResourceType type = it->first;
            int needed = it->second;

            auto myResIt = m_fixedResources.find(type);
            int owned = (myResIt != m_fixedResources.end()) ? myResIt->second : 0;

            if (owned >= needed) {
                it = deficit.erase(it);
            } else {
                it->second -= owned;
                ++it;
            }
        }

        // --- [NEW] 科技标记减费逻辑 ---
        int discountCount = 0;
        if (m_progressTokens.count(ProgressToken::MASONRY) && targetType == CardType::CIVILIAN) {
            discountCount = Config::MASONRY_DISCOUNT;
        } else if (m_progressTokens.count(ProgressToken::ARCHITECTURE) && targetType == CardType::WONDER) {
            discountCount = Config::ARCHITECTURE_DISCOUNT;
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
            if (m_coins < cost.getCoins()) return { false, cost.getCoins() };
            return { true, cost.getCoins() };
        }

        // 4. 利用多选一资源填补剩余缺口 (寻找最小交易费)
        int minTradingCost = std::numeric_limits<int>::max();

        solveMinCost(deficit, 0, m_choiceResources, opponent, *this, minTradingCost);

        // 5. 汇总结果
        int totalRequired = cost.getCoins() + minTradingCost;
        bool canAfford = (m_coins >= totalRequired);

        return { canAfford, totalRequired };
    }

    // --- 动作执行 (Mutators) ---

    void Player::payCoins(int amount) {
        m_coins = std::max(0, m_coins - amount);
    }

    void Player::gainCoins(int amount) {
        m_coins += amount;
    }

    void Player::setTradingDiscount(ResourceType r, bool active) {
        m_tradingDiscounts[r] = active;
    }

    void Player::addClaimedSciencePair(ScienceSymbol s) {
        m_claimedSciencePairs.insert(s);
    }

    void Player::addResource(ResourceType type, int count, bool isTradable) {
        m_fixedResources[type] += count;
        if (isTradable) {
            m_publicProduction[type] += count;
        }
    }

    void Player::addProductionChoice(const std::vector<ResourceType>& choices) {
        m_choiceResources.push_back(choices);
    }

    void Player::addScienceSymbol(ScienceSymbol s) {
        if (s != ScienceSymbol::NONE) {
            m_scienceSymbols[s]++;
        }
    }

    void Player::addChainTag(const std::string& tag) {
        if (!tag.empty()) m_ownedChainTags.insert(tag);
    }

    void Player::addProgressToken(ProgressToken token) {
        m_progressTokens.insert(token);
        // 立即生效的 buff 处理 (如 LAW)
        if (token == ProgressToken::LAW) addScienceSymbol(ScienceSymbol::LAW);
    }

    void Player::constructCard(Card* card) {
        m_builtCards.push_back(card);
        addChainTag(card->getChainTag());
    }

    Card* Player::removeCardByType(CardType type) {
        auto it = std::find_if(m_builtCards.rbegin(), m_builtCards.rend(),
            [type](Card* c){ return c->getType() == type; });

        if (it != m_builtCards.rend()) {
            Card* c = *it;
            // Convert reverse iterator to forward iterator for erase
            // rbegin is last element, rend is before first.
            // base() of reverse_iterator returns the element *after* the one it points to.
            // So if it points to element X, base() points to X+1.
            // To get X, we need (it + 1).base()
            
            auto fwdIt = (it + 1).base();
            m_builtCards.erase(fwdIt);
            return c;
        }
        return nullptr;
    }

    void Player::addUnbuiltWonder(Wonder* w) {
        m_unbuiltWonders.push_back(w);
    }

    void Player::removeUnbuiltWonder(const std::string& wonderId) {
        auto it = std::remove_if(m_unbuiltWonders.begin(), m_unbuiltWonders.end(),
            [&](Wonder* w){ return w->getId() == wonderId; });
        if (it != m_unbuiltWonders.end()) {
            m_unbuiltWonders.erase(it, m_unbuiltWonders.end());
        }
    }

    void Player::clearUnbuiltWonders() {
        m_unbuiltWonders.clear();
    }

    void Player::constructWonder(std::string wonderId, Card* overlayCard) {
        auto it = std::find_if(m_unbuiltWonders.begin(), m_unbuiltWonders.end(),
            [&](Wonder* w){ return w->getId() == wonderId; });

        if (it != m_unbuiltWonders.end()) {
            Wonder* w = *it;
            w->build(overlayCard);
            m_builtWonders.push_back(w);
            m_unbuiltWonders.erase(it);
        }
    }

}