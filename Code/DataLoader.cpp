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
    std::vector<std::shared_ptr<IEffect>> parseEffects(const Value& vList) {
        std::vector<std::shared_ptr<IEffect>> effects;
        const auto& list = vList.asList();

        for (const auto& effVal : list) {
            std::string type = effVal["type"].asString();

            if (type == "PRODUCTION" || type == "PRODUCTION_CHOICE") {
                std::map<ResourceType, int> res;
                const auto& resObj = effVal["resources"].asObject();
                // 如果是 PRODUCTION_CHOICE, json可能是 array ["WOOD", "CLAY"]，或者 map
                // 根据 gamedata.json 定义：
                // PRODUCTION: { "resources": { "WOOD": 1 } }
                // PRODUCTION_CHOICE: { "resources": ["GLASS", "PAPER"] } -> 我们的 TinyJson 处理这个会有点 tricky
                // 让我们兼容这两种写法。

                if (effVal["resources"].isList()) {
                    // 列表格式 ["A", "B"]，默认为各 1 个
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
                effects.push_back(std::make_shared<MilitaryEffect>(effVal["shields"].asInt()));
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
                // 特殊处理 WONDER 类型，它在 strToCardType 可能返回默认
                if (effVal["target_type"].asString() == "WONDER") t = CardType::WONDER;

                bool countWonder = (t == CardType::WONDER);
                // 修正逻辑：CoinsPerTypeEffect 的构造参数需要对齐
                // 如果 target_type 是 WONDER，我们传入 CardType::WONDER (需在 Global.h 确认 CardType 定义)
                // 或者我们复用 CardType::WONDER 并在 Player::getCardCount 处理

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
                else c = GuildCriteria::YELLOW_CARDS; // Default

                effects.push_back(std::make_shared<GuildEffect>(c));
            }
        }
        return effects;
    }

    bool DataLoader::loadFromFile(const std::string& filepath,
                                  std::vector<Card>& outCards,
                                  std::vector<Wonder>& outWonders)
    {
        // --- 新增调试代码 ---
        std::cout << "[DEBUG] Current Working Directory: " << std::filesystem::current_path() << std::endl;
        std::cout << "[DEBUG] Trying to open: " << filepath << std::endl;
        if (!std::filesystem::exists(filepath)) {
            std::cerr << "[DEBUG] Error: The file does not exist at this path!" << std::endl;
        }
        // ------------------
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
            c.requiresChainTag = v["requires_chain"].asString(); // 注意 json 字段可能不一致，需检查 json
            // 修正：我的 json 示例里写的是 "chainTag" 还是 "provides_chain"?
            // 之前的 json 示例用了 "provides_chain" 和 "requires_chain" (马厩示例)
            // 让我们保持一致。

            c.effects = parseEffects(v["effects"]);
            outCards.push_back(c);
        }

        // 2. Load Wonders
        const auto& wondersList = root["wonders"].asList();
        for (const auto& v : wondersList) {
            Wonder w;
            w.id = v["id"].asString();
            w.name = v["name"].asString();
            w.cost = parseCost(v["cost"]);
            w.effects = parseEffects(v["effects"]);
            outWonders.push_back(w);
        }

        std::cout << "Loaded " << outCards.size() << " cards and " << outWonders.size() << " wonders." << std::endl;
        return true;
    }

}