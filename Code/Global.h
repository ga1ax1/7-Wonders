//
// Created by choyichi on 2025/12/15.
//

#ifndef SEVEN_WONDERS_DUEL_GLOBAL_H
#define SEVEN_WONDERS_DUEL_GLOBAL_H

#include <vector>
#include <string>
#include <map>
#include <memory>
#include <algorithm>

namespace SevenWondersDuel {

    // 控制台颜色代码 (用于界面美化)
    enum class ConsoleColor {
        DEFAULT, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, GREY, BROWN
    };

    // 资源类型
    enum class ResourceType {
        WOOD, STONE, CLAY,   // 棕色原料
        PAPER, GLASS         // 灰色加工品
    };

    // 卡牌类型
    enum class CardType {
        RAW_MATERIAL,   // 棕色
        MANUFACTURED,   // 灰色
        CIVILIAN,       // 蓝色
        SCIENTIFIC,     // 绿色
        COMMERCIAL,     // 黄色
        MILITARY,       // 红色
        GUILD,          // 紫色 (时代III专用)
        WONDER          // 奇迹
    };

    // 科学符号类型
    enum class ScienceSymbol {
        NONE,       // 无
        GLOBE,      // 地球仪
        TABLET,     // 石板
        MORTAR,     // 研钵(烧杯)
        COMPASS,    // 圆规
        WHEEL,      // 齿轮
        QUILL,      // 羽毛笔
        LAW         // 法律 (发展标记提供)
    };

    // 游戏主状态机
    enum class GameState {
        WONDER_DRAFT_PHASE_1,      // 第一轮奇迹轮抽
        WONDER_DRAFT_PHASE_2,      // 第二轮奇迹轮抽
        AGE_PLAY_PHASE,            // 玩家行动阶段

        // 中断/交互状态
        WAITING_FOR_TOKEN_SELECTION_PAIR, // 凑齐2个相同符号，选标记
        WAITING_FOR_TOKEN_SELECTION_LIB,  // 图书馆：3选1
        WAITING_FOR_DESTRUCTION,          // 摧毁对手卡牌
        WAITING_FOR_DISCARD_BUILD,        // 摩索拉斯陵墓：弃牌堆建造
        WAITING_FOR_START_PLAYER_SELECTION, // 决定先手

        GAME_OVER
    };

    // 玩家行动指令类型
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

    // 发展标记
    enum class ProgressToken {
        NONE,
        AGRICULTURE,   // 农业
        URBANISM,      // 都市化
        STRATEGY,      // 战略
        THEOLOGY,      // 宗教
        ECONOMY,       // 经济
        MASONRY,       // 砖石
        ARCHITECTURE,  // 建筑
        LAW,           // 法律
        MATHEMATICS,   // 数学
        PHILOSOPHY     // 哲学
    };

    enum class VictoryType { NONE, MILITARY, SCIENCE, CIVILIAN };

    // 辅助工具：将字符串转换为枚举
    inline ResourceType strToResource(const std::string& s) {
        if(s == "WOOD") return ResourceType::WOOD;
        if(s == "STONE") return ResourceType::STONE;
        if(s == "CLAY") return ResourceType::CLAY;
        if(s == "PAPER") return ResourceType::PAPER;
        if(s == "GLASS") return ResourceType::GLASS;
        return ResourceType::WOOD; // Default
    }

    inline CardType strToCardType(const std::string& s) {
        if(s == "RAW_MATERIAL") return CardType::RAW_MATERIAL;
        if(s == "MANUFACTURED") return CardType::MANUFACTURED;
        if(s == "MILITARY") return CardType::MILITARY;
        if(s == "SCIENTIFIC") return CardType::SCIENTIFIC;
        if(s == "COMMERCIAL") return CardType::COMMERCIAL;
        if(s == "CIVILIAN") return CardType::CIVILIAN;
        if(s == "GUILD") return CardType::GUILD;
        return CardType::CIVILIAN;
    }

    inline ScienceSymbol strToScienceSymbol(const std::string& s) {
        if(s == "GLOBE") return ScienceSymbol::GLOBE;
        if(s == "TABLET") return ScienceSymbol::TABLET;
        if(s == "MORTAR") return ScienceSymbol::MORTAR;
        if(s == "COMPASS") return ScienceSymbol::COMPASS;
        if(s == "WHEEL") return ScienceSymbol::WHEEL;
        if(s == "QUILL") return ScienceSymbol::QUILL;
        return ScienceSymbol::NONE;
    }

    // 简单的转换函数用于 UI 显示
    inline std::string resourceToString(ResourceType r) {
        switch(r) {
            case ResourceType::WOOD: return "Wood";
            case ResourceType::STONE: return "Stone";
            case ResourceType::CLAY: return "Clay";
            case ResourceType::PAPER: return "Paper";
            case ResourceType::GLASS: return "Glass";
            default: return "";
        }
    }
}

#endif // SEVEN_WONDERS_DUEL_GLOBAL_H