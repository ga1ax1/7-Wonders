#include "Board.h"
#include "Player.h"
#include <algorithm>
#include <cmath>

namespace SevenWondersDuel {

    // ==========================================================
    //  MilitaryTrack
    // ==========================================================

    std::vector<int> MilitaryTrack::move(int shields, int currentPlayerId) {
        std::vector<int> lootEvents;

        // P0 (Id=0) 向正方向(+)推，P1 (Id=1) 向负方向(-)推
        int direction = (currentPlayerId == 0) ? 1 : -1;
        int startPos = m_position;
        m_position += (shields * direction);

        // 钳制范围
        if (m_position > Config::MILITARY_THRESHOLD_WIN) m_position = Config::MILITARY_THRESHOLD_WIN;
        if (m_position < -Config::MILITARY_THRESHOLD_WIN) m_position = -Config::MILITARY_THRESHOLD_WIN;

        // 检查掠夺 (跨越阈值)
        // P1 (右侧玩家) 被攻击 (Position > 0)
        if (startPos < Config::MILITARY_THRESHOLD_LOOT_1 && m_position >= Config::MILITARY_THRESHOLD_LOOT_1 && m_lootTokens[2]) {
            m_lootTokens[2] = false;
            lootEvents.push_back(Config::MILITARY_LOOT_VALUE_1);
        }
        if (startPos < Config::MILITARY_THRESHOLD_LOOT_2 && m_position >= Config::MILITARY_THRESHOLD_LOOT_2 && m_lootTokens[3]) {
            m_lootTokens[3] = false;
            lootEvents.push_back(Config::MILITARY_LOOT_VALUE_2);
        }

        // P0 (左侧玩家) 被攻击 (Position < 0)
        if (startPos > -Config::MILITARY_THRESHOLD_LOOT_1 && m_position <= -Config::MILITARY_THRESHOLD_LOOT_1 && m_lootTokens[0]) {
            m_lootTokens[0] = false;
            lootEvents.push_back(-Config::MILITARY_LOOT_VALUE_1);
        }
        if (startPos > -Config::MILITARY_THRESHOLD_LOOT_2 && m_position <= -Config::MILITARY_THRESHOLD_LOOT_2 && m_lootTokens[1]) {
            m_lootTokens[1] = false;
            lootEvents.push_back(-Config::MILITARY_LOOT_VALUE_2);
        }

        return lootEvents;
    }

    int MilitaryTrack::getVictoryPoints(int playerId) const {
        int absPos = std::abs(m_position);
        int points = 0;
        if (absPos >= 1 && absPos < Config::MILITARY_THRESHOLD_LOOT_1) points = 0;
        else if (absPos >= Config::MILITARY_THRESHOLD_LOOT_1 && absPos < Config::MILITARY_THRESHOLD_LOOT_2) points = Config::MILITARY_VP_LEVEL_1;
        else if (absPos >= Config::MILITARY_THRESHOLD_LOOT_2 && absPos < Config::MILITARY_THRESHOLD_WIN) points = Config::MILITARY_VP_LEVEL_2;
        else if (absPos >= Config::MILITARY_THRESHOLD_WIN) points = Config::MILITARY_VP_WIN;

        if (m_position > 0 && playerId == 0) return points;
        if (m_position < 0 && playerId == 1) return points;
        return 0;
    }

    // ==========================================================
    //  CardPyramid
    // ==========================================================

    void CardPyramid::init(int age, const std::vector<Card*>& deck) {
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


    Card* CardPyramid::removeCard(const std::string& cardId) {
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

    void CardPyramid::addSlot(int row, int count, bool faceUp, const std::vector<Card*>& deck, int& deckIdx) {
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

    int CardPyramid::getAbsIndex(CardSlot* ptr) {
        return static_cast<int>(ptr - &m_slots[0]);
    }

    std::vector<CardSlot*> CardPyramid::getSlotsByRow(int r) {
        std::vector<CardSlot*> res;
        for (auto& s : m_slots) {
            if (s.getRow() == r) res.push_back(&s);
        }
        return res;
    }

    void CardPyramid::setupDependenciesAge1() {
        for (int r = 0; r < 4; ++r) {
            auto upper = getSlotsByRow(r);
            auto lower = getSlotsByRow(r+1);

            for (int k = 0; k < (int)upper.size(); ++k) {
                if (k < (int)lower.size()) upper[k]->addCoveredBy(getAbsIndex(lower[k]));
                if (k+1 < (int)lower.size()) upper[k]->addCoveredBy(getAbsIndex(lower[k+1]));
            }
        }
    }

    void CardPyramid::setupDependenciesAge2() {
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

    void CardPyramid::setupDependenciesAge3() {
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

    // ==========================================================
    //  Board
    // ==========================================================

    std::vector<int> Board::moveMilitary(int shields, int currentPlayerId) {
        return m_militaryTrack.move(shields, currentPlayerId);
    }

    void Board::initPyramid(int age, const std::vector<Card*>& deck) {
        m_cardStructure.init(age, deck);
    }

    Card* Board::removeCardFromPyramid(const std::string& cardId) {
        return m_cardStructure.removeCard(cardId);
    }

    void Board::addToDiscardPile(Card* c) {
        if (c) m_discardPile.push_back(c);
    }

    Card* Board::removeCardFromDiscardPile(const std::string& cardId) {
        auto it = std::find_if(m_discardPile.begin(), m_discardPile.end(), 
            [&](Card* c){ return c->getId() == cardId; });
        
        if (it != m_discardPile.end()) {
            Card* c = *it;
            m_discardPile.erase(it);
            return c;
        }
        return nullptr;
    }

    void Board::setAvailableProgressTokens(const std::vector<ProgressToken>& tokens) {
        m_availableProgressTokens = tokens;
    }
    
    void Board::setBoxProgressTokens(const std::vector<ProgressToken>& tokens) {
        m_boxProgressTokens = tokens;
    }

    void Board::addAvailableProgressToken(ProgressToken t) {
        m_availableProgressTokens.push_back(t);
    }
    
    void Board::addBoxProgressToken(ProgressToken t) {
        m_boxProgressTokens.push_back(t);
    }

    bool Board::removeAvailableProgressToken(ProgressToken t) {
        auto it = std::find(m_availableProgressTokens.begin(), m_availableProgressTokens.end(), t);
        if (it != m_availableProgressTokens.end()) {
            m_availableProgressTokens.erase(it);
            return true;
        }
        return false;
    }
    
    bool Board::removeBoxProgressToken(ProgressToken t) {
        auto it = std::find(m_boxProgressTokens.begin(), m_boxProgressTokens.end(), t);
        if (it != m_boxProgressTokens.end()) {
            m_boxProgressTokens.erase(it);
            return true;
        }
        return false;
    }

    void Board::destroyCard(Player* target, CardType color) {
        Card* removed = target->removeCardByType(color);
        if (removed) {
            m_discardPile.push_back(removed);
        }
    }

}
