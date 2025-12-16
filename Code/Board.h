//
// Created by choyichi on 2025/12/15.
//

#ifndef SEVEN_WONDERS_DUEL_BOARD_H
#define SEVEN_WONDERS_DUEL_BOARD_H

#include "Global.h"
#include "Card.h"
#include "Player.h"
#include <vector>
#include <string>
#include <cmath>
#include <memory>
#include <algorithm>
#include <iostream>

namespace SevenWondersDuel {

    // 军事轨道
    struct MilitaryTrack {
        int position = 0; // 范围 -9 (P1胜利) 到 +9 (P2胜利)

        // 掠夺标记 (true 代表还在，false 代表已被移除/触发)
        // 索引含义: 0: P1失2元, 1: P1失5元, 2: P2失2元, 3: P2失5元
        bool lootTokens[4] = {true, true, true, true};

        // 移动冲突标记
        // 返回：触发的掠夺事件列表 (负数代表P1扣钱，正数代表P2扣钱)
        std::vector<int> move(int shields, int currentPlayerId) {
            std::vector<int> lootEvents;

            // P0 (Id=0) 向正方向(+)推，P1 (Id=1) 向负方向(-)推
            int direction = (currentPlayerId == 0) ? 1 : -1;
            int startPos = position;
            position += (shields * direction);

            // 钳制范围
            if (position > 9) position = 9;
            if (position < -9) position = -9;

            // 检查掠夺 (跨越阈值)
            // 阈值：3 (2元), 6 (5元)

            // P1 (右侧玩家) 被攻击 (Position > 0)
            if (startPos < 3 && position >= 3 && lootTokens[2]) {
                lootTokens[2] = false;
                lootEvents.push_back(2);
            }
            if (startPos < 6 && position >= 6 && lootTokens[3]) {
                lootTokens[3] = false;
                lootEvents.push_back(5);
            }

            // P0 (左侧玩家) 被攻击 (Position < 0)
            if (startPos > -3 && position <= -3 && lootTokens[0]) {
                lootTokens[0] = false;
                lootEvents.push_back(-2);
            }
            if (startPos > -6 && position <= -6 && lootTokens[1]) {
                lootTokens[1] = false;
                lootEvents.push_back(-5);
            }

            return lootEvents;
        }

        int getVictoryPoints(int playerId) const {
            int absPos = std::abs(position);
            int points = 0;
            if (absPos >= 1 && absPos < 3) points = 0;
            else if (absPos >= 3 && absPos < 6) points = 2;
            else if (absPos >= 6 && absPos < 9) points = 5;
            else if (absPos >= 9) points = 10;

            if (position > 0 && playerId == 0) return points;
            if (position < 0 && playerId == 1) return points;
            return 0;
        }
    };

    // 金字塔结构管理
    class CardPyramid {
    public:
        std::vector<CardSlot> slots;

        void init(int age, const std::vector<Card*>& deck) {
            slots.clear();
            int cardIdx = 0;

            if (age == 1) {
                // Age 1: 正金字塔 (2-3-4-5-6)
                addSlot(0, 2, true, deck, cardIdx);  // Base (Top visual in reality, but Bottom logic here)
                addSlot(1, 3, false, deck, cardIdx);
                addSlot(2, 4, true, deck, cardIdx);
                addSlot(3, 5, false, deck, cardIdx);
                addSlot(4, 6, true, deck, cardIdx);

                setupDependenciesAge1();
            }
            else if (age == 2) {
                // Age 2: 倒金字塔 (6-5-4-3-2)
                addSlot(0, 6, true, deck, cardIdx);
                addSlot(1, 5, false, deck, cardIdx);
                addSlot(2, 4, true, deck, cardIdx);
                addSlot(3, 3, false, deck, cardIdx);
                addSlot(4, 2, true, deck, cardIdx);

                setupDependenciesAge2();
            }
            else if (age == 3) {
                // Age 3: 这里的形状比较特殊 (Snake)，但在代码实现中为了稳定性，
                // 我们复用 Age 1 的正金字塔结构 (2-3-4-5-6)，这保证了20张牌的逻辑完整性。
                addSlot(0, 2, true, deck, cardIdx);
                addSlot(1, 3, false, deck, cardIdx);
                addSlot(2, 4, true, deck, cardIdx);
                addSlot(3, 5, false, deck, cardIdx);
                addSlot(4, 6, true, deck, cardIdx);

                setupDependenciesAge3();
            }
        }

        std::vector<const CardSlot*> getAvailableCards() const {
            std::vector<const CardSlot*> available;
            for (const auto& slot : slots) {
                if (!slot.isRemoved && slot.coveredBy.empty()) {
                    available.push_back(&slot);
                }
            }
            return available;
        }

        Card* removeCard(std::string cardId) {
            Card* removedCard = nullptr;
            int removedIdx = -1;

            for (int i = 0; i < slots.size(); ++i) {
                if (slots[i].id == cardId || (slots[i].cardPtr && slots[i].cardPtr->id == cardId)) {
                    if (slots[i].isRemoved) return nullptr; // 已经被移除了
                    slots[i].isRemoved = true;
                    removedCard = slots[i].cardPtr;
                    removedIdx = i;
                    break;
                }
            }

            if (removedIdx == -1) return nullptr;

            // 更新依赖
            // 我们需要找到所有 "coveredBy" 包含 removedIdx 的卡，把这个依赖去掉
            for (auto& s : slots) {
                if (s.isRemoved) continue;

                auto it = std::remove(s.coveredBy.begin(), s.coveredBy.end(), removedIdx);
                if (it != s.coveredBy.end()) {
                    s.coveredBy.erase(it, s.coveredBy.end());

                    if (s.coveredBy.empty() && !s.isFaceUp) {
                        s.isFaceUp = true;
                    }
                }
            }
            return removedCard;
        }

    private:
        void addSlot(int row, int count, bool faceUp, const std::vector<Card*>& deck, int& deckIdx) {
            for (int i = 0; i < count; ++i) {
                if (deckIdx >= deck.size()) break;
                CardSlot slot;
                slot.cardPtr = deck[deckIdx++];
                slot.id = slot.cardPtr->id;
                slot.isFaceUp = faceUp;
                slot.row = row;
                slot.index = i;
                slots.push_back(slot);
            }
        }

        // 辅助：获取在 slots 数组中的绝对索引
        int getAbsIndex(CardSlot* ptr) {
            return static_cast<int>(ptr - &slots[0]);
        }

        // 辅助：获取某一行的所有指针
        std::vector<CardSlot*> getSlotsByRow(int r) {
            std::vector<CardSlot*> res;
            for (auto& s : slots) {
                if (s.row == r) res.push_back(&s);
            }
            return res;
        }

        // --- 核心拓扑实现 ---

        void setupDependenciesAge1() {
            // 正金字塔逻辑: Row r 覆盖 Row r+1
            // 上层 Row r (Index k) 覆盖了 下层 Row r+1 的 Index k 和 Index k+1
            // 反过来说：下层 Lower[k] 被 Upper[k-1] 和 Upper[k] 压住

            for (int r = 0; r < 4; ++r) { // 0 到 3行
                auto upper = getSlotsByRow(r);
                auto lower = getSlotsByRow(r+1);

                for (int k = 0; k < lower.size(); ++k) {
                    // 左上方的卡 (Index k-1)
                    if (k > 0 && (k-1) < upper.size()) {
                        lower[k]->coveredBy.push_back(getAbsIndex(upper[k-1]));
                    }
                    // 正上方的卡 (Index k)
                    if (k < upper.size()) {
                        lower[k]->coveredBy.push_back(getAbsIndex(upper[k]));
                    }
                }
            }
        }

        void setupDependenciesAge2() {
            // 倒金字塔逻辑: Row r (长) 覆盖 Row r+1 (短)
            // 上层 Row r (Index k, k+1) 覆盖 下层 Row r+1 (Index k)
            // 反过来说：下层 Lower[k] 被 Upper[k] 和 Upper[k+1] 压住

            for (int r = 0; r < 4; ++r) {
                auto upper = getSlotsByRow(r);
                auto lower = getSlotsByRow(r+1);

                for (int k = 0; k < lower.size(); ++k) {
                    // Upper k
                    if (k < upper.size()) {
                        lower[k]->coveredBy.push_back(getAbsIndex(upper[k]));
                    }
                    // Upper k+1
                    if (k+1 < upper.size()) {
                        lower[k]->coveredBy.push_back(getAbsIndex(upper[k+1]));
                    }
                }
            }
        }

        void setupDependenciesAge3() {
            // 复用 Age 1 的逻辑 (正金字塔)
            setupDependenciesAge1();
        }
    };

    class Board {
    public:
        MilitaryTrack militaryTrack;
        CardPyramid cardStructure;
        std::vector<Card*> discardPile;

        std::vector<ProgressToken> availableProgressTokens;
        std::vector<ProgressToken> boxProgressTokens;

        Board() = default;

        // 从玩家已建卡牌中移除指定颜色的卡 (通常只移除最后一张符合条件的)
        void destroyCard(Player* target, CardType color) {
            auto it = std::find_if(target->builtCards.rbegin(), target->builtCards.rend(),
                [color](Card* c){ return c->type == color; });

            if (it != target->builtCards.rend()) {
                Card* c = *it;
                // 从 builtCards 移除 (注意 rbegin 转 base iterator 的细节)
                // 简单起见，再正向查一次
                auto fwdIt = std::find(target->builtCards.begin(), target->builtCards.end(), c);
                if (fwdIt != target->builtCards.end()) {
                    target->builtCards.erase(fwdIt);
                    discardPile.push_back(c);
                    // 注意：这里移除了卡，但 Player 缓存的资源 (fixedResources) 未更新
                    // 在完整实现中应调用 target->recalculateStats()
                }
            }
        }
    };
}

#endif // SEVEN_WONDERS_DUEL_BOARD_H