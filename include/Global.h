//
// Created by choyichi on 2025/12/15.
//

#ifndef SEVEN_WONDERS_DUEL_GLOBAL_H
#define SEVEN_WONDERS_DUEL_GLOBAL_H

#include <vector>
#include <string>
#include <map>
#include <memory>

namespace SevenWondersDuel {

    enum class ConsoleColor {
        DEFAULT, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, GREY, BROWN
    };

    enum class ResourceType {
        WOOD, STONE, CLAY,
        PAPER, GLASS
    };

    enum class CardType {
        RAW_MATERIAL,
        MANUFACTURED,
        CIVILIAN,
        SCIENTIFIC,
        COMMERCIAL,
        MILITARY,
        GUILD,
        WONDER
    };

    enum class ScienceSymbol {
        NONE,
        GLOBE,
        TABLET,
        MORTAR,
        COMPASS,
        WHEEL,
        QUILL,
        LAW
    };

    enum class GameState {
        WONDER_DRAFT_PHASE_1,
        WONDER_DRAFT_PHASE_2,
        AGE_PLAY_PHASE,
        WAITING_FOR_TOKEN_SELECTION_PAIR,
        WAITING_FOR_TOKEN_SELECTION_LIB,
        WAITING_FOR_DESTRUCTION,
        WAITING_FOR_DISCARD_BUILD,
        WAITING_FOR_START_PLAYER_SELECTION,
        GAME_OVER
    };

    enum class ActionType {
        DRAFT_WONDER,
        BUILD_CARD,
        DISCARD_FOR_COINS,
        BUILD_WONDER,
        SELECT_PROGRESS_TOKEN,
        SELECT_DESTRUCTION,
        SELECT_FROM_DISCARD,
        CHOOSE_STARTING_PLAYER
    };

    enum class ProgressToken {
        NONE,
        AGRICULTURE,
        URBANISM,
        STRATEGY,
        THEOLOGY,
        ECONOMY,
        MASONRY,
        ARCHITECTURE,
        LAW,
        MATHEMATICS,
        PHILOSOPHY
    };

    struct Action {
        ActionType type;
        std::string targetCardId;
        std::string targetWonderId;
        ProgressToken selectedToken = ProgressToken::NONE;
        ResourceType chosenResource = ResourceType::WOOD;
    };

    struct ActionResult {
        bool isValid;
        int cost = 0;
        std::string message;
    };

    enum class VictoryType { NONE, MILITARY, SCIENCE, CIVILIAN };

    namespace Config {
        static constexpr int INITIAL_COINS = 7;
        static constexpr int COINS_PER_VP = 3;
        static constexpr int BASE_DISCARD_GAIN = 2;
        
        static constexpr int MASONRY_DISCOUNT = 2;
        static constexpr int ARCHITECTURE_DISCOUNT = 2;
        
        static constexpr int URBANISM_CHAIN_BONUS = 4;
        static constexpr int URBANISM_TOKEN_BONUS = 6;
        
        static constexpr int AGRICULTURE_VP = 4;
        static constexpr int PHILOSOPHY_VP = 7;
        static constexpr int MATHEMATICS_VP_PER_TOKEN = 3;

        static constexpr int MILITARY_THRESHOLD_LOOT_1 = 3;
        static constexpr int MILITARY_THRESHOLD_LOOT_2 = 6;
        static constexpr int MILITARY_THRESHOLD_WIN = 9;
        static constexpr int MILITARY_LOOT_VALUE_1 = 2;
        static constexpr int MILITARY_LOOT_VALUE_2 = 5;
        static constexpr int MILITARY_VP_LEVEL_1 = 2;
        static constexpr int MILITARY_VP_LEVEL_2 = 5;
        static constexpr int MILITARY_VP_WIN = 10;

        static constexpr int SCIENCE_WIN_THRESHOLD = 6;
        static constexpr int SCIENCE_PAIR_COUNT = 2;

        static constexpr int TRADING_BASE_COST = 2;
        static constexpr int MAX_TOTAL_WONDERS = 7;
    }

    ResourceType strToResource(const std::string& s);
    CardType strToCardType(const std::string& s);
    ScienceSymbol strToScienceSymbol(const std::string& s);
    std::string resourceToString(ResourceType r);
}

#endif // SEVEN_WONDERS_DUEL_GLOBAL_H
