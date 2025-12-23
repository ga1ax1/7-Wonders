#include "GameController.h"
#include <algorithm>

namespace SevenWondersDuel {

    // ==========================================================
    //  GameModel
    // ==========================================================

    GameModel::GameModel() {
        m_board = std::make_unique<Board>();
    }

    void GameModel::removeFromDraftPool(const std::string& wonderId) {
        m_draftPool.erase(std::remove_if(m_draftPool.begin(), m_draftPool.end(),
            [&](Wonder* w){ return w->getId() == wonderId; }), m_draftPool.end());
    }

    void GameModel::popRemainingWonder() {
        if (!m_remainingWonders.empty()) m_remainingWonders.pop_back();
    }

    Wonder* GameModel::backRemainingWonder() {
        return m_remainingWonders.empty() ? nullptr : m_remainingWonders.back();
    }

    void GameModel::populateData(std::vector<Card> cards, std::vector<Wonder> wonders) {
        m_allCards = std::move(cards);
        m_allWonders = std::move(wonders);
    }

    Card* GameModel::findCardById(const std::string& id) {
        for(auto& c : m_allCards) if(c.getId() == id) return &c;
        return nullptr;
    }

    const Card* GameModel::findCardById(const std::string& id) const {
        for(const auto& c : m_allCards) if(c.getId() == id) return &c;
        return nullptr;
    }

    Wonder* GameModel::findWonderById(const std::string& id) {
        for(auto& w : m_allWonders) if(w.getId() == id) return &w;
        return nullptr;
    }

    const Wonder* GameModel::findWonderById(const std::string& id) const {
        for(const auto& w : m_allWonders) if(w.getId() == id) return &w;
        return nullptr;
    }

    std::vector<Wonder*> GameModel::getPointersToAllWonders() {
        std::vector<Wonder*> res;
        for(auto& w : m_allWonders) res.push_back(&w);
        return res;
    }

    void GameModel::addLog(const std::string& msg) {
        m_gameLog.push_back(msg);
    }

    void GameModel::clearLog() {
        m_gameLog.clear();
    }

    int GameModel::getRemainingCardCount() const {
        int count = 0;
        for(const auto& slot : m_board->getCardStructure().getSlots()) {
            if (!slot.isRemoved()) count++;
        }
        return count;
    }

}
