//
// Created by choyichi on 2026/01/02.
//

#ifndef SEVEN_WONDERS_DUEL_CARDBUILDER_H
#define SEVEN_WONDERS_DUEL_CARDBUILDER_H

#include "Card.h"
#include <string>
#include <vector>
#include <memory>

namespace SevenWondersDuel {

    class CardBuilder {
    private:
        Card m_card;
        std::vector<std::shared_ptr<IEffect>> m_tempEffects;

    public:
        CardBuilder() = default;

        CardBuilder& withId(const std::string& id);
        CardBuilder& withName(const std::string& name);
        CardBuilder& withAge(int age);
        CardBuilder& withType(CardType type);
        CardBuilder& withCost(const ResourceCost& cost);
        CardBuilder& withChainTag(const std::string& tag);
        CardBuilder& withRequiresChainTag(const std::string& tag);
        
        CardBuilder& addEffect(std::shared_ptr<IEffect> effect);
        CardBuilder& setEffects(std::vector<std::shared_ptr<IEffect>> effects);

        Card build();
    };

}

#endif // SEVEN_WONDERS_DUEL_CARDBUILDER_H
