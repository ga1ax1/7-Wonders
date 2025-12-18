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

        cost.coins = v["coins"].asInt();

        const auto& resObj = v["resources"].asObject();
        for (const auto& [key, val] : resObj) {
            cost.resources[strToResource(key)] = val.asInt();
        }
        return cost;
    }

    // 辅助函数：解析 Effect 列表
    // 新增参数 isFromCard：用于区分效果来源是卡牌还是奇迹 (影响 Military Effect 的 Strategy 逻辑)
    std::vector<std::shared_ptr<IEffect>> parseEffects(const Value& vList, bool isFromCard) {
        std::vector<std::shared_ptr<IEffect>> effects;
        const auto& list = vList.asList();

        for (const auto& effVal : list) {
            std::string type = effVal["type"].asString();

            if (type == "PRODUCTION" || type == "PRODUCTION_CHOICE") {
                std::map<ResourceType, int> res;
                const auto& resObj = effVal["resources"].asObject();

                if (effVal["resources"].isList()) {
                    for(const auto& item : effVal["resources"].asList()) {
                        res[strToResource(item.asString())] = 1;
                    }
                } else {
                     for (const auto& [key, val] : resObj) {
                        res[strToResource(key)] = val.asInt();
                    }
                }

                bool isChoice = (type == "PRODUCTION_CHOICE");
                effects.push_back(std::make_shared<ProductionEffect>(res, isChoice));
            }
            else if (type == "MILITARY") {
                // 传入 isFromCard 标记
                effects.push_back(std::make_shared<MilitaryEffect>(effVal["shields"].asInt(), isFromCard));
            }
            else if (type == "VICTORY_POINTS") {
                effects.push_back(std::make_shared<VictoryPointEffect>(effVal["amount"].asInt()));
            }
            else if (type == "SCIENCE") {
                effects.push_back(std::make_shared<ScienceEffect>(strToScienceSymbol(effVal["symbol"].asString())));
            }
            else if (type == "COINS") {
                effects.push_back(std::make_shared<CoinEffect>(effVal["amount"].asInt()));
            }
            else if (type == "TRADE_DISCOUNT") {
                effects.push_back(std::make_shared<TradeDiscountEffect>(strToResource(effVal["resource"].asString())));
            }
            else if (type == "COINS_PER_TYPE") {
                CardType t = strToCardType(effVal["target_type"].asString());
                if (effVal["target_type"].asString() == "WONDER") t = CardType::WONDER;

                bool countWonder = (t == CardType::WONDER);
                effects.push_back(std::make_shared<CoinsPerTypeEffect>(t, effVal["amount"].asInt(), countWonder));
            }
            else if (type == "DESTROY_CARD") {
                effects.push_back(std::make_shared<DestroyCardEffect>(strToCardType(effVal["target_color"].asString())));
            }
            else if (type == "EXTRA_TURN") {
                effects.push_back(std::make_shared<ExtraTurnEffect>());
            }
            else if (type == "BUILD_FROM_DISCARD") {
                effects.push_back(std::make_shared<BuildFromDiscardEffect>());
            }
            else if (type == "PROGRESS_TOKEN_SELECT") {
                effects.push_back(std::make_shared<ProgressTokenSelectEffect>());
            }
            else if (type == "OPPONENT_LOSE_COINS") {
                effects.push_back(std::make_shared<OpponentLoseCoinsEffect>(effVal["amount"].asInt()));
            }
            else if (type == "GUILD") {
                std::string criteriaStr = effVal["criteria"].asString();
                GuildCriteria c;
                if(criteriaStr == "YELLOW_CARDS") c = GuildCriteria::YELLOW_CARDS;
                else if(criteriaStr == "BROWN_GREY_CARDS") c = GuildCriteria::BROWN_GREY_CARDS;
                else if(criteriaStr == "WONDERS") c = GuildCriteria::WONDERS;
                else if(criteriaStr == "BLUE_CARDS") c = GuildCriteria::BLUE_CARDS;
                else if(criteriaStr == "GREEN_CARDS") c = GuildCriteria::GREEN_CARDS;
                else if(criteriaStr == "RED_CARDS") c = GuildCriteria::RED_CARDS;
                else if(criteriaStr == "COINS") c = GuildCriteria::COINS;
                else c = GuildCriteria::YELLOW_CARDS;

                effects.push_back(std::make_shared<GuildEffect>(c));
            }
        }
        return effects;
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
            c.id = v["id"].asString();
            c.name = v["name"].asString();
            c.age = v["age"].asInt();
            c.type = strToCardType(v["type"].asString());
            c.cost = parseCost(v["cost"]);
            c.chainTag = v["provided_chain"].asString();
            c.requiresChainTag = v["requires_chain"].asString();

            // 解析卡牌效果 (Source = Card, True)
            c.effects = parseEffects(v["effects"], true);
            outCards.push_back(c);
        }

        // 2. Load Wonders
        const auto& wondersList = root["wonders"].asList();
        for (const auto& v : wondersList) {
            Wonder w;
            w.id = v["id"].asString();
            w.name = v["name"].asString();
            w.cost = parseCost(v["cost"]);

            // 解析奇迹效果 (Source = Wonder, False)
            w.effects = parseEffects(v["effects"], false);
            outWonders.push_back(w);
        }

        std::cout << "Loaded " << outCards.size() << " cards and " << outWonders.size() << " wonders." << std::endl;
        return true;
    }

}