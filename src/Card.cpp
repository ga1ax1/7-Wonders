#include "Card.h"
#include <algorithm>

namespace SevenWondersDuel {

    // ==========================================================
    //  CardSlot
    // ==========================================================

    bool CardSlot::notifyCoveringRemoved(int index) {
        auto it = std::remove(m_coveredBy.begin(), m_coveredBy.end(), index);
        if (it != m_coveredBy.end()) {
            m_coveredBy.erase(it, m_coveredBy.end());
            if (m_coveredBy.empty()) {
                if (!m_isFaceUp) {
                    m_isFaceUp = true;
                    return true;
                }
            }
        }
        return false;
    }

    // ==========================================================
    //  Card
    // ==========================================================

    int Card::getVictoryPoints(const Player* self, const Player* opponent) const {
        int total = 0;
        for(const auto& eff : m_effects) {
            total += eff->calculateScore(self, opponent);
        }
        return total;
    }

    // ==========================================================
    //  Wonder
    // ==========================================================

    void Wonder::build(const Card* overlay) {
        m_isBuilt = true;
        m_builtOverlayCard = overlay;
    }

    void Wonder::reset() {
        m_isBuilt = false;
        m_builtOverlayCard = nullptr;
    }

    int Wonder::getVictoryPoints(const Player* self, const Player* opponent) const {
        if (!m_isBuilt) return 0;
        int total = 0;
        for(const auto& eff : m_effects) {
            total += eff->calculateScore(self, opponent);
        }
        return total;
    }

}
