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
                // Row 0 是塔尖 (被遮挡最多), Row 4 是塔底 (开放)
                addSlot(0, 2, true, deck, cardIdx);
                addSlot(1, 3, false, deck, cardIdx);
                addSlot(2, 4, true, deck, cardIdx);
                addSlot(3, 5, false, deck, cardIdx);
                addSlot(4, 6, true, deck, cardIdx);

                setupDependenciesAge1();
            }
            else if (age == 2) {
                // Age 2: 倒金字塔 (6-5-4-3-2)
                // Row 0 是顶部 (开放), Row 4 是底部尖端 (被遮挡)
                addSlot(0, 6, true, deck, cardIdx);
                addSlot(1, 5, false, deck, cardIdx);
                addSlot(2, 4, true, deck, cardIdx);
                addSlot(3, 3, false, deck, cardIdx);
                addSlot(4, 2, true, deck, cardIdx);

                setupDependenciesAge2();
            }
            else if (age == 3) {
                // Age 3: 蛇形结构 (20 cards)
                // 布局设计 (2-3-4-2-3-4-2)
                // Row 0: 2 (U) - Covered by Row 1
                // Row 1: 3 (D) - Covered by Row 2
                // Row 2: 4 (U) - Covered by Row 3
                // Row 3: 2 (D) - Covered by Row 4
                // Row 4: 3 (U) - Covered by Row 5
                // Row 5: 4 (D) - Covered by Row 6
                // Row 6: 2 (U) - Open

                addSlot(0, 2, true, deck, cardIdx);
                addSlot(1, 3, false, deck, cardIdx);
                addSlot(2, 4, true, deck, cardIdx);
                addSlot(3, 2, false, deck, cardIdx);
                addSlot(4, 3, true, deck, cardIdx);
                addSlot(5, 4, false, deck, cardIdx);
                addSlot(6, 2, true, deck, cardIdx);

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
            // 修正：正金字塔逻辑
            // Row 4 (6 cards) 是底部，完全开放 (Free)。
            // Row 3 (5 cards) 被 Row 4 遮挡。
            // Row r 被 Row r+1 遮挡。
            // 遮挡逻辑：Upper[k] 被 Lower[k] 和 Lower[k+1] 遮挡

            for (int r = 0; r < 4; ++r) { // 0 到 3行
                auto upper = getSlotsByRow(r);   // Row r
                auto lower = getSlotsByRow(r+1); // Row r+1 (Base side)

                for (int k = 0; k < upper.size(); ++k) {
                    // Upper[k] 被 Lower[k] 遮挡
                    if (k < lower.size()) {
                        upper[k]->coveredBy.push_back(getAbsIndex(lower[k]));
                    }
                    // Upper[k] 被 Lower[k+1] 遮挡
                    if (k+1 < lower.size()) {
                        upper[k]->coveredBy.push_back(getAbsIndex(lower[k+1]));
                    }
                }
            }
        }

        void setupDependenciesAge2() {
            // 倒金字塔逻辑: Row 0 (6 cards) 是顶部，完全开放。
            // Row 1 (5 cards) 被 Row 0 遮挡。
            // Row r+1 被 Row r 遮挡。
            // 遮挡逻辑：Lower[k] 被 Upper[k] 和 Upper[k+1] 遮挡 (此逻辑未变，因为之前的实现就是对的)
            // 之前实现：Upper=Row r, Lower=Row r+1. Lower covered by Upper.

            for (int r = 0; r < 4; ++r) {
                auto upper = getSlotsByRow(r);
                auto lower = getSlotsByRow(r+1);

                for (int k = 0; k < lower.size(); ++k) {
                    // Lower[k] 依赖 Upper[k]
                    if (k < upper.size()) {
                        lower[k]->coveredBy.push_back(getAbsIndex(upper[k]));
                    }
                    // Lower[k] 依赖 Upper[k+1]
                    if (k+1 < upper.size()) {
                        lower[k]->coveredBy.push_back(getAbsIndex(upper[k+1]));
                    }
                }
            }
        }

        void setupDependenciesAge3() {
            // Age 3 结构 (Row 0 -> Row 6):
            // Row 0: 2 cards (Face Up)   <- Top
            // Row 1: 3 cards (Face Down)
            // Row 2: 4 cards (Face Up)
            // Row 3: 2 cards (Face Down) <- Neck
            // Row 4: 3 cards (Face Up)
            // Row 5: 4 cards (Face Down)
            // Row 6: 2 cards (Face Up)   <- Bottom (Available at start)

            for (int r = 0; r < 6; ++r) {
                auto upper = getSlotsByRow(r);     // 被遮挡层
                auto lower = getSlotsByRow(r + 1); // 遮挡层 (Blocking cards)

                if (r == 0) {
                    // 2 covered by 3 (Standard Expansion)
                    // U0 <- L0, L1
                    // U1 <- L1, L2
                    if(upper.size() > 0 && lower.size() > 1) {
                        upper[0]->coveredBy.push_back(getAbsIndex(lower[0]));
                        upper[0]->coveredBy.push_back(getAbsIndex(lower[1]));
                    }
                    if(upper.size() > 1 && lower.size() > 2) {
                        upper[1]->coveredBy.push_back(getAbsIndex(lower[1]));
                        upper[1]->coveredBy.push_back(getAbsIndex(lower[2]));
                    }
                }
                else if (r == 1) {
                    // 3 covered by 4 (Standard Expansion)
                    for(int i=0; i<3; ++i) {
                         if (upper.size() > i && lower.size() > i+1) {
                             upper[i]->coveredBy.push_back(getAbsIndex(lower[i]));
                             upper[i]->coveredBy.push_back(getAbsIndex(lower[i+1]));
                         }
                    }
                }
                else if (r == 2) {
                    // 4 covered by 2 (Narrowing / Neck)
                    // 左边2张(0,1) 被 L0 遮挡
                    // 右边2张(2,3) 被 L1 遮挡
                    if (lower.size() > 0) {
                        if(upper.size() > 0) upper[0]->coveredBy.push_back(getAbsIndex(lower[0]));
                        if(upper.size() > 1) upper[1]->coveredBy.push_back(getAbsIndex(lower[0]));
                    }
                    if (lower.size() > 1) {
                        if(upper.size() > 2) upper[2]->coveredBy.push_back(getAbsIndex(lower[1]));
                        if(upper.size() > 3) upper[3]->coveredBy.push_back(getAbsIndex(lower[1]));
                    }
                }
                else if (r == 3) {
                    // 2 covered by 3 (Widening)
                    if(upper.size() > 0 && lower.size() > 1) {
                        upper[0]->coveredBy.push_back(getAbsIndex(lower[0]));
                        upper[0]->coveredBy.push_back(getAbsIndex(lower[1]));
                    }
                    if(upper.size() > 1 && lower.size() > 2) {
                        upper[1]->coveredBy.push_back(getAbsIndex(lower[1]));
                        upper[1]->coveredBy.push_back(getAbsIndex(lower[2]));
                    }
                }
                else if (r == 4) {
                    // 3 covered by 4 (Standard Expansion)
                    for(int i=0; i<3; ++i) {
                         if (upper.size() > i && lower.size() > i+1) {
                             upper[i]->coveredBy.push_back(getAbsIndex(lower[i]));
                             upper[i]->coveredBy.push_back(getAbsIndex(lower[i+1]));
                         }
                    }
                }
                else if (r == 5) {
                    // 4 covered by 2 (Narrowing to Bottom)
                    // 同 r==2 的逻辑
                    if (lower.size() > 0) {
                        if(upper.size() > 0) upper[0]->coveredBy.push_back(getAbsIndex(lower[0]));
                        if(upper.size() > 1) upper[1]->coveredBy.push_back(getAbsIndex(lower[0]));
                    }
                    if (lower.size() > 1) {
                        if(upper.size() > 2) upper[2]->coveredBy.push_back(getAbsIndex(lower[1]));
                        if(upper.size() > 3) upper[3]->coveredBy.push_back(getAbsIndex(lower[1]));
                    }
                }
            }
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