//
// Created by choyichi on 2025/12/15.
//

#ifndef SEVEN_WONDERS_DUEL_BOARD_H
#define SEVEN_WONDERS_DUEL_BOARD_H

#include "Global.h"
#include "Card.h"
#include <vector>
#include <string>
#include <memory>
#include <iterator>

namespace SevenWondersDuel {

    class Player;

    // 军事轨道
    class MilitaryTrack {
    private:
        int m_position = 0; // 范围 -9 (P1胜利) 到 +9 (P2胜利)
        bool m_lootTokens[4] = {true, true, true, true};

    public:
        int getPosition() const { return m_position; }
        const bool* getLootTokens() const { return m_lootTokens; }
        
        std::vector<int> move(int shields, int currentPlayerId);
        int getVictoryPoints(int playerId) const;
    };

    // 金字塔结构管理
    class CardPyramid {
    private:
        std::vector<CardSlot> m_slots;

    public:
        const std::vector<CardSlot>& getSlots() const { return m_slots; }
        
        void init(int age, const std::vector<Card*>& deck);
        // std::vector<const CardSlot*> getAvailableCards() const; // Removed in favor of Iterator
        Card* removeCard(const std::string& cardId);

        // --- Iterator Implementation ---
        class Iterator {
        public:
            using iterator_category = std::forward_iterator_tag;
            using difference_type   = std::ptrdiff_t;
            using value_type        = const CardSlot;
            using pointer           = const CardSlot*;
            using reference         = const CardSlot&;

            Iterator(const std::vector<CardSlot>* slots, int index) 
                : m_ptrSlots(slots), m_currentIndex(index) {
                advanceToNextAvailable();
            }

            reference operator*() const { return (*m_ptrSlots)[m_currentIndex]; }
            pointer operator->() const { return &(*m_ptrSlots)[m_currentIndex]; }

            Iterator& operator++() {
                m_currentIndex++;
                advanceToNextAvailable();
                return *this;
            }

            Iterator operator++(int) {
                Iterator tmp = *this;
                ++(*this);
                return tmp;
            }

            friend bool operator==(const Iterator& a, const Iterator& b) {
                return a.m_currentIndex == b.m_currentIndex && a.m_ptrSlots == b.m_ptrSlots;
            }

            friend bool operator!=(const Iterator& a, const Iterator& b) {
                return !(a == b);
            }

        private:
            const std::vector<CardSlot>* m_ptrSlots;
            int m_currentIndex;

            void advanceToNextAvailable() {
                while (m_currentIndex < (int)m_ptrSlots->size()) {
                    const CardSlot& slot = (*m_ptrSlots)[m_currentIndex];
                    // If slot is NOT removed AND NOT covered, it is valid. Stop here.
                    if (!slot.isRemoved() && slot.getCoveredBy().empty()) {
                        break;
                    }
                    m_currentIndex++;
                }
            }
        };

        Iterator begin() const { return Iterator(&m_slots, 0); }
        Iterator end() const { return Iterator(&m_slots, (int)m_slots.size()); }

    private:
        void addSlot(int row, int count, bool faceUp, const std::vector<Card*>& deck, int& deckIdx);
        int getAbsIndex(CardSlot* ptr);
        std::vector<CardSlot*> getSlotsByRow(int r);

        void setupDependenciesAge1();
        void setupDependenciesAge2();
        void setupDependenciesAge3();
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

        const MilitaryTrack& getMilitaryTrack() const { return m_militaryTrack; }
        const CardPyramid& getCardStructure() const { return m_cardStructure; }
        const std::vector<Card*>& getDiscardPile() const { return m_discardPile; }
        const std::vector<ProgressToken>& getAvailableProgressTokens() const { return m_availableProgressTokens; }
        const std::vector<ProgressToken>& getBoxProgressTokens() const { return m_boxProgressTokens; }

        std::vector<int> moveMilitary(int shields, int currentPlayerId);
        void initPyramid(int age, const std::vector<Card*>& deck);
        Card* removeCardFromPyramid(const std::string& cardId);
        void addToDiscardPile(Card* c);
        Card* removeCardFromDiscardPile(const std::string& cardId);

        void setAvailableProgressTokens(const std::vector<ProgressToken>& tokens);
        void setBoxProgressTokens(const std::vector<ProgressToken>& tokens);
        void addAvailableProgressToken(ProgressToken t);
        void addBoxProgressToken(ProgressToken t);
        bool removeAvailableProgressToken(ProgressToken t);
        bool removeBoxProgressToken(ProgressToken t);

        void destroyCard(Player* target, CardType color);
    };
}

#endif // SEVEN_WONDERS_DUEL_BOARD_H