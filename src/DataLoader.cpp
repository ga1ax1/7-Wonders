//
// Created by choyichi on 2025/12/16.
//

#include "DataLoader.h"
#include "TinyJson.h"
#include "EffectSystem.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>

namespace SevenWondersDuel {

    using namespace TinyJson;

    // 辅助函数：解析 Cost 对象
    ResourceCost parseCost(const Value& v) {
        ResourceCost cost;
        if (v.asObject().empty()) return cost;

        cost.setCoins(v["coins"].asInt());

        const auto& resObj = v["resources"].asObject();
        for (const auto& [key, val] : resObj) {
            cost.addResource(strToResource(key), val.asInt());
        }
        return cost;
    }

    bool DataLoader::loadFromFile(const std::string& filepath,
                                  std::vector<Card>& outCards,
                                  std::vector<Wonder>& outWonders)
    {
        std::cout << "[DEBUG] Loading data from: " << filepath << std::endl;
        if (!std::filesystem::exists(filepath)) {
            std::cerr << "[DEBUG] Error: File not found!" << std::endl;
        }

        std::ifstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "Failed to open " << filepath << std::endl;
            return false;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string jsonContent = buffer.str();

        Value root = Parser::parse(jsonContent);
        if (!root.isObject()) {
            std::cerr << "Invalid JSON root" << std::endl;
            return false;
        }

        // 1. Load Cards
        const auto& cardsList = root["cards"].asList();
        for (const auto& v : cardsList) {
            Card c;
            c.setId(v["id"].asString());
            c.setName(v["name"].asString());
            c.setAge(v["age"].asInt());
            c.setType(strToCardType(v["type"].asString()));
            c.setCost(parseCost(v["cost"]));
            c.setChainTag(v["provides_chain"].asString());
            c.setRequiresChainTag(v["requires_chain"].asString());

            // 解析卡牌效果 (Source = Card, True)
            // 使用工厂模式创建效果
            c.setEffects(EffectFactory::createEffects(v["effects"], c.getType(), true));
            outCards.push_back(c);
        }

        // 2. Load Wonders
        const auto& wondersList = root["wonders"].asList();
        for (const auto& v : wondersList) {
            Wonder w;
            w.setId(v["id"].asString());
            w.setName(v["name"].asString());
            w.setCost(parseCost(v["cost"]));

            // 解析奇迹效果 (Source = Wonder, False)
            // 使用工厂模式创建效果
            w.setEffects(EffectFactory::createEffects(v["effects"], CardType::WONDER, false));
            outWonders.push_back(w);
        }

        std::cout << "Loaded " << outCards.size() << " cards and " << outWonders.size() << " wonders." << std::endl;
        return true;
    }

}