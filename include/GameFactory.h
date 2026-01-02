//
// Created by choyichi on 2026/01/02.
//

#ifndef SEVEN_WONDERS_DUEL_GAMEFACTORY_H
#define SEVEN_WONDERS_DUEL_GAMEFACTORY_H

#include "Card.h"
#include "Global.h"
#include <nlohmann/json.hpp>
#include <vector>
#include <string>

namespace SevenWondersDuel {

    class IGameFactory {
    public:
        virtual ~IGameFactory() = default;
        virtual std::vector<Card> createCards() = 0;
        virtual std::vector<Wonder> createWonders() = 0;
        virtual std::vector<ProgressToken> createAvailableTokens() = 0;
        virtual std::vector<ProgressToken> createBoxTokens() = 0;
    };

    class BaseGameFactory : public IGameFactory {
    private:
        nlohmann::json m_jsonData;
        std::vector<ProgressToken> m_shuffledTokens;
        
        // Helper to parse cost from JSON value
        ResourceCost parseCost(const nlohmann::json& v);

    public:
        BaseGameFactory(const std::string& jsonPath);
        ~BaseGameFactory() override;
        std::vector<Card> createCards() override;
        std::vector<Wonder> createWonders() override;
        std::vector<ProgressToken> createAvailableTokens() override;
        std::vector<ProgressToken> createBoxTokens() override;
    };

}

#endif // SEVEN_WONDERS_DUEL_GAMEFACTORY_H
