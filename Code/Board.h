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
    class MilitaryTrack {
    private:
        int m_position = 0; // 范围 -9 (P1胜利) 到 +9 (P2胜利)

        // 掠夺标记 (true 代表还在，false 代表已被移除/触发)
        // 索引含义: 0: P1失2元, 1: P1失5元, 2: P2失2元, 3: P2失5元
        bool m_lootTokens[4] = {true, true, true, true};

    public:
        // Getters
        int getPosition() const { return m_position; }
        const bool* getLootTokens() const { return m_lootTokens; }
        
        // 移动冲突标记
        // 返回：触发的掠夺事件列表 (负数代表P1扣钱，正数代表P2扣钱)
        std::vector<int> move(int shields, int currentPlayerId) {
            std::vector<int> lootEvents;

            // P0 (Id=0) 向正方向(+)推，P1 (Id=1) 向负方向(-)推
            int direction = (currentPlayerId == 0) ? 1 : -1;
            int startPos = m_position;
            m_position += (shields * direction);

            // 钳制范围
            if (m_position > 9) m_position = 9;
            if (m_position < -9) m_position = -9;

            // 检查掠夺 (跨越阈值)
            // 阈值：3 (2元), 6 (5元)

            // P1 (右侧玩家) 被攻击 (Position > 0)
            if (startPos < 3 && m_position >= 3 && m_lootTokens[2]) {
                m_lootTokens[2] = false;
                lootEvents.push_back(2);
            }
            if (startPos < 6 && m_position >= 6 && m_lootTokens[3]) {
                m_lootTokens[3] = false;
                lootEvents.push_back(5);
            }

            // P0 (左侧玩家) 被攻击 (Position < 0)
            if (startPos > -3 && m_position <= -3 && m_lootTokens[0]) {
                m_lootTokens[0] = false;
                lootEvents.push_back(-2);
            }
            if (startPos > -6 && m_position <= -6 && m_lootTokens[1]) {
                m_lootTokens[1] = false;
                lootEvents.push_back(-5);
            }

            return lootEvents;
        }

        int getVictoryPoints(int playerId) const {
            int absPos = std::abs(m_position);
            int points = 0;
            if (absPos >= 1 && absPos < 3) points = 0;
            else if (absPos >= 3 && absPos < 6) points = 2;
            else if (absPos >= 6 && absPos < 9) points = 5;
            else if (absPos >= 9) points = 10;

            if (m_position > 0 && playerId == 0) return points;
            if (m_position < 0 && playerId == 1) return points;
            return 0;
        }
    };

    // 金字塔结构管理
    class CardPyramid {
    private:
        std::vector<CardSlot> m_slots;

    public:
        // Accessors
        const std::vector<CardSlot>& getSlots() const { return m_slots; }
        
        void init(int age, const std::vector<Card*>& deck) {
            m_slots.clear();
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
                addSlot(0, 2, true, deck, cardIdx);
                addSlot(1, 3, false, deck, cardIdx);
                addSlot(2, 4, true, deck, cardIdx);
                addSlot(3, 2, false, deck, cardIdx); 
                addSlot(4, 4, true, deck, cardIdx);
                addSlot(5, 3, false, deck, cardIdx);
                addSlot(6, 2, true, deck, cardIdx); 

                setupDependenciesAge3();
            }
        }

        std::vector<const CardSlot*> getAvailableCards() const {
            std::vector<const CardSlot*> available;
            for (const auto& slot : m_slots) {
                if (!slot.isRemoved() && slot.getCoveredBy().empty()) {
                    available.push_back(&slot);
                }
            }
            return available;
        }

        Card* removeCard(const std::string& cardId) {
            Card* removedCard = nullptr;
            int removedIdx = -1;

            for (int i = 0; i < (int)m_slots.size(); ++i) {
                if (m_slots[i].getId() == cardId || (m_slots[i].getCardPtr() && m_slots[i].getCardPtr()->getId() == cardId)) {
                    if (m_slots[i].isRemoved()) return nullptr; // 已经被移除了
                    m_slots[i].setRemoved(true);
                    removedCard = m_slots[i].getCardPtr();
                    removedIdx = i;
                    break;
                }
            }

            if (removedIdx == -1) return nullptr;

            // 更新依赖
            for (auto& s : m_slots) {
                if (!s.isRemoved()) {
                    s.notifyCoveringRemoved(removedIdx);
                }
            }
            return removedCard;
        }

    private:
        void addSlot(int row, int count, bool faceUp, const std::vector<Card*>& deck, int& deckIdx) {
            for (int i = 0; i < count; ++i) {
                if (deckIdx >= (int)deck.size()) break;
                CardSlot slot;
                slot.setCardPtr(deck[deckIdx++]);
                slot.setId(slot.getCardPtr()->getId());
                slot.setFaceUp(faceUp);
                slot.setRow(row);
                slot.setIndex(i);
                m_slots.push_back(slot);
            }
        }

        int getAbsIndex(CardSlot* ptr) {
            return static_cast<int>(ptr - &m_slots[0]);
        }

        std::vector<CardSlot*> getSlotsByRow(int r) {
            std::vector<CardSlot*> res;
            for (auto& s : m_slots) {
                if (s.getRow() == r) res.push_back(&s);
            }
            return res;
        }

        // --- 核心拓扑实现 ---

        void setupDependenciesAge1() {
            for (int r = 0; r < 4; ++r) {
                auto upper = getSlotsByRow(r);
                auto lower = getSlotsByRow(r+1);

                for (int k = 0; k < (int)upper.size(); ++k) {
                    if (k < (int)lower.size()) upper[k]->addCoveredBy(getAbsIndex(lower[k]));
                    if (k+1 < (int)lower.size()) upper[k]->addCoveredBy(getAbsIndex(lower[k+1]));
                }
            }
        }

        void setupDependenciesAge2() {
            for (int r = 0; r < 4; ++r) {
                auto upper = getSlotsByRow(r);   // 宽层 (被遮挡)
                auto lower = getSlotsByRow(r+1); // 窄层 (遮挡者)

                for (int k = 0; k < (int)lower.size(); ++k) {
                    if (k < (int)upper.size()) {
                        upper[k]->addCoveredBy(getAbsIndex(lower[k]));
                    }
                    if (k+1 < (int)upper.size()) {
                        upper[k+1]->addCoveredBy(getAbsIndex(lower[k]));
                    }
                }
            }
        }

        void setupDependenciesAge3() {
            for (int r = 0; r < 6; ++r) {
                auto upper = getSlotsByRow(r);     // 被遮挡
                auto lower = getSlotsByRow(r + 1); // 遮挡者

                if (r == 2) {
                    if (lower.size() > 0 && upper.size() > 1) {
                        upper[0]->addCoveredBy(getAbsIndex(lower[0]));
                        upper[1]->addCoveredBy(getAbsIndex(lower[0]));
                    }
                    if (lower.size() > 1 && upper.size() > 3) {
                        upper[2]->addCoveredBy(getAbsIndex(lower[1]));
                        upper[3]->addCoveredBy(getAbsIndex(lower[1]));
                    }
                }
                else if (r == 3) {
                    if (upper.size() > 0 && lower.size() > 1) {
                        upper[0]->addCoveredBy(getAbsIndex(lower[0]));
                        upper[0]->addCoveredBy(getAbsIndex(lower[1]));
                    }
                    if (upper.size() > 1 && lower.size() > 3) {
                        upper[1]->addCoveredBy(getAbsIndex(lower[2]));
                        upper[1]->addCoveredBy(getAbsIndex(lower[3]));
                    }
                }
                else if (upper.size() > lower.size()) {
                    for (int k = 0; k < (int)lower.size(); ++k) {
                        if (k < (int)upper.size()) upper[k]->addCoveredBy(getAbsIndex(lower[k]));
                        if (k+1 < (int)upper.size()) upper[k+1]->addCoveredBy(getAbsIndex(lower[k]));
                    }
                }
                else if (upper.size() < lower.size()) {
                    for (int k = 0; k < (int)upper.size(); ++k) {
                        if (k < (int)lower.size()) upper[k]->addCoveredBy(getAbsIndex(lower[k]));
                        if (k+1 < (int)lower.size()) upper[k]->addCoveredBy(getAbsIndex(lower[k+1]));
                    }
                }
                else {
                     for (int k = 0; k < (int)upper.size(); ++k) {
                        upper[k]->addCoveredBy(getAbsIndex(lower[k]));
                    }
                }
            }
        }
    };

    class Board {
    private:
        MilitaryTrack m_militaryTrack;
        CardPyramid m_cardStructure;
        std::vector<Card*> m_discardPile;

        std::vector<ProgressToken> m_availableProgressTokens;
        std::vector<ProgressToken> m_boxProgressTokens;

    public:
        Board() = default;

        // Getters (Const)
        const MilitaryTrack& getMilitaryTrack() const { return m_militaryTrack; }
        const CardPyramid& getCardStructure() const { return m_cardStructure; }
        const std::vector<Card*>& getDiscardPile() const { return m_discardPile; }
        const std::vector<ProgressToken>& getAvailableProgressTokens() const { return m_availableProgressTokens; }
        const std::vector<ProgressToken>& getBoxProgressTokens() const { return m_boxProgressTokens; }

        // --- Mutators (Encapsulated) ---

        // Military
        std::vector<int> moveMilitary(int shields, int currentPlayerId) {
            return m_militaryTrack.move(shields, currentPlayerId);
        }

        // Pyramid
        void initPyramid(int age, const std::vector<Card*>& deck) {
            m_cardStructure.init(age, deck);
        }

        Card* removeCardFromPyramid(const std::string& cardId) {
            return m_cardStructure.removeCard(cardId);
        }
        
        // Discard Pile
        void addToDiscardPile(Card* c) {
            if (c) m_discardPile.push_back(c);
        }

        Card* removeCardFromDiscardPile(const std::string& cardId) {
            auto it = std::find_if(m_discardPile.begin(), m_discardPile.end(), 
                [&](Card* c){ return c->getId() == cardId; });
            
            if (it != m_discardPile.end()) {
                Card* c = *it;
                m_discardPile.erase(it);
                return c;
            }
            return nullptr;
        }

        // Progress Tokens
        void setAvailableProgressTokens(const std::vector<ProgressToken>& tokens) {
            m_availableProgressTokens = tokens;
        }
        
        void setBoxProgressTokens(const std::vector<ProgressToken>& tokens) {
            m_boxProgressTokens = tokens;
        }

        void addAvailableProgressToken(ProgressToken t) {
            m_availableProgressTokens.push_back(t);
        }
        
        void addBoxProgressToken(ProgressToken t) {
            m_boxProgressTokens.push_back(t);
        }

        bool removeAvailableProgressToken(ProgressToken t) {
            auto it = std::find(m_availableProgressTokens.begin(), m_availableProgressTokens.end(), t);
            if (it != m_availableProgressTokens.end()) {
                m_availableProgressTokens.erase(it);
                return true;
            }
            return false;
        }
        
        bool removeBoxProgressToken(ProgressToken t) {
            auto it = std::find(m_boxProgressTokens.begin(), m_boxProgressTokens.end(), t);
            if (it != m_boxProgressTokens.end()) {
                m_boxProgressTokens.erase(it);
                return true;
            }
            return false;
        }


        // 从玩家已建卡牌中移除指定颜色的卡
        void destroyCard(Player* target, CardType color) {
            Card* removed = target->removeCardByType(color);
            if (removed) {
                m_discardPile.push_back(removed);
            }
        }
    };
}

#endif // SEVEN_WONDERS_DUEL_BOARD_H
