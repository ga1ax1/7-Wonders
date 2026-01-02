//
// Created by choyichi on 2026/01/02.
//

#include "CardBuilder.h"

namespace SevenWondersDuel {

    CardBuilder& CardBuilder::withId(const std::string& id) {
        m_card.setId(id);
        return *this;
    }

    CardBuilder& CardBuilder::withName(const std::string& name) {
        m_card.setName(name);
        return *this;
    }

    CardBuilder& CardBuilder::withAge(int age) {
        m_card.setAge(age);
        return *this;
    }

    CardBuilder& CardBuilder::withType(CardType type) {
        m_card.setType(type);
        return *this;
    }

    CardBuilder& CardBuilder::withCost(const ResourceCost& cost) {
        m_card.setCost(cost);
        return *this;
    }

    CardBuilder& CardBuilder::withChainTag(const std::string& tag) {
        m_card.setChainTag(tag);
        return *this;
    }

    CardBuilder& CardBuilder::withRequiresChainTag(const std::string& tag) {
        m_card.setRequiresChainTag(tag);
        return *this;
    }

    CardBuilder& CardBuilder::addEffect(std::shared_ptr<IEffect> effect) {
        m_tempEffects.push_back(effect);
        return *this;
    }

    CardBuilder& CardBuilder::setEffects(std::vector<std::shared_ptr<IEffect>> effects) {
        m_tempEffects = std::move(effects);
        return *this;
    }

    Card CardBuilder::build() {
        m_card.setEffects(std::move(m_tempEffects));
        return m_card;
    }

}
