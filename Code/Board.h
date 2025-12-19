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
                addSlot(0, 2, true, deck, cardIdx);
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
                // Age 3: 蛇形结构 (20 cards)
                // 结构 (从顶到底): 2(U) - 3(D) - 4(U) - 2(D) - 4(U) - 3(D) - 2(U)
                // Row 3 (index) 是那行 Face Down 的 2 张卡
                addSlot(0, 2, true, deck, cardIdx);
                addSlot(1, 3, false, deck, cardIdx);
                addSlot(2, 4, true, deck, cardIdx);
                addSlot(3, 2, false, deck, cardIdx); // Row 4 (user index)
                addSlot(4, 4, true, deck, cardIdx);
                addSlot(5, 3, false, deck, cardIdx);
                addSlot(6, 2, true, deck, cardIdx);  // Row 7 (user index), Initially Available

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
            for (auto& s : slots) {
                if (s.isRemoved) continue;

                auto it = std::remove(s.coveredBy.begin(), s.coveredBy.end(), removedIdx);
                if (it != s.coveredBy.end()) {
                    s.coveredBy.erase(it, s.coveredBy.end());

                    // [核心机制] 翻牌逻辑
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

        int getAbsIndex(CardSlot* ptr) {
            return static_cast<int>(ptr - &slots[0]);
        }

        std::vector<CardSlot*> getSlotsByRow(int r) {
            std::vector<CardSlot*> res;
            for (auto& s : slots) {
                if (s.row == r) res.push_back(&s);
            }
            return res;
        }

        // --- 核心拓扑实现 ---

        void setupDependenciesAge1() {
            // 正金字塔：Row r 被 Row r+1 遮挡
            // 底部 (Row 4) 开放
            for (int r = 0; r < 4; ++r) {
                auto upper = getSlotsByRow(r);
                auto lower = getSlotsByRow(r+1);

                for (int k = 0; k < upper.size(); ++k) {
                    if (k < lower.size()) upper[k]->coveredBy.push_back(getAbsIndex(lower[k]));
                    if (k+1 < lower.size()) upper[k]->coveredBy.push_back(getAbsIndex(lower[k+1]));
                }
            }
        }

        void setupDependenciesAge2() {
            // Age 2: 倒金字塔 (6-5-4-3-2)
            // 逻辑要求：下层卡 (Row r+1, 窄) 压制 上层卡 (Row r, 宽)。
            // 结果：Row 4 (2 cards) 完全开放，Row 0 (6 cards) 埋在最下面。

            for (int r = 0; r < 4; ++r) {
                auto upper = getSlotsByRow(r);   // 宽层 (被遮挡)
                auto lower = getSlotsByRow(r+1); // 窄层 (遮挡者)

                for (int k = 0; k < lower.size(); ++k) {
                    // Upper[k] 被 Lower[k] 遮挡
                    if (k < upper.size()) {
                        upper[k]->coveredBy.push_back(getAbsIndex(lower[k]));
                    }
                    // Upper[k+1] 被 Lower[k] 遮挡
                    if (k+1 < upper.size()) {
                        upper[k+1]->coveredBy.push_back(getAbsIndex(lower[k]));
                    }
                }
            }
        }

        void setupDependenciesAge3() {
            // Age 3: 蛇形结构
            // 结构：2(U)-3(D)-4(U)-2(D)-4(U)-3(D)-2(U)
            // 逻辑：下层 (Row r+1) 遮挡 上层 (Row r)

            for (int r = 0; r < 6; ++r) {
                auto upper = getSlotsByRow(r);     // 被遮挡
                auto lower = getSlotsByRow(r + 1); // 遮挡者

                // Row 2 (4 cards) covered by Row 3 (2 cards) -> Split Logic
                if (r == 2) {
                    // L[0] blocks U[0], U[1]
                    // L[1] blocks U[2], U[3]
                    if (lower.size() > 0 && upper.size() > 1) {
                        upper[0]->coveredBy.push_back(getAbsIndex(lower[0]));
                        upper[1]->coveredBy.push_back(getAbsIndex(lower[0]));
                    }
                    if (lower.size() > 1 && upper.size() > 3) {
                        upper[2]->coveredBy.push_back(getAbsIndex(lower[1]));
                        upper[3]->coveredBy.push_back(getAbsIndex(lower[1]));
                    }
                }
                // Row 3 (2 cards) covered by Row 4 (4 cards) -> Split Logic
                else if (r == 3) {
                    // U[0] blocked by L[0], L[1]
                    // U[1] blocked by L[2], L[3]
                    if (upper.size() > 0 && lower.size() > 1) {
                        upper[0]->coveredBy.push_back(getAbsIndex(lower[0]));
                        upper[0]->coveredBy.push_back(getAbsIndex(lower[1]));
                    }
                    if (upper.size() > 1 && lower.size() > 3) {
                        upper[1]->coveredBy.push_back(getAbsIndex(lower[2]));
                        upper[1]->coveredBy.push_back(getAbsIndex(lower[3]));
                    }
                }
                // 其他情况：通用逻辑
                else if (upper.size() > lower.size()) {
                    // 宽行被窄行遮挡 (如 3 被 2 遮，4 被 3 遮)
                    // Lower[k] 遮挡 Upper[k] 和 Upper[k+1]
                    for (int k = 0; k < lower.size(); ++k) {
                        if (k < upper.size()) upper[k]->coveredBy.push_back(getAbsIndex(lower[k]));
                        if (k+1 < upper.size()) upper[k+1]->coveredBy.push_back(getAbsIndex(lower[k]));
                    }
                }
                else if (upper.size() < lower.size()) {
                    // 窄行被宽行遮挡 (如 2 被 3 遮，3 被 4 遮)
                    // Upper[k] 被 Lower[k] 和 Lower[k+1] 遮挡
                    for (int k = 0; k < upper.size(); ++k) {
                        if (k < lower.size()) upper[k]->coveredBy.push_back(getAbsIndex(lower[k]));
                        if (k+1 < lower.size()) upper[k]->coveredBy.push_back(getAbsIndex(lower[k+1]));
                    }
                }
                else {
                    // 等宽
                     for (int k = 0; k < upper.size(); ++k) {
                        upper[k]->coveredBy.push_back(getAbsIndex(lower[k]));
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

        // 从玩家已建卡牌中移除指定颜色的卡
        void destroyCard(Player* target, CardType color) {
            auto it = std::find_if(target->builtCards.rbegin(), target->builtCards.rend(),
                [color](Card* c){ return c->type == color; });

            if (it != target->builtCards.rend()) {
                Card* c = *it;
                auto fwdIt = std::find(target->builtCards.begin(), target->builtCards.end(), c);
                if (fwdIt != target->builtCards.end()) {
                    target->builtCards.erase(fwdIt);
                    discardPile.push_back(c);
                }
            }
        }
    };
}

#endif // SEVEN_WONDERS_DUEL_BOARD_H