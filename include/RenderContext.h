#ifndef SEVEN_WONDERS_DUEL_RENDERCONTEXT_H
#define SEVEN_WONDERS_DUEL_RENDERCONTEXT_H

#include "Global.h"
#include <map>
#include <string>
#include <vector>

namespace SevenWondersDuel {

    // 渲染上下文：用于将显示的短ID (1, 2...) 映射回字符串 ID
    struct RenderContext {
        std::map<int, std::string> cardIdMap;     // 金字塔卡牌
        std::map<int, std::string> wonderIdMap;   // 奇迹
        std::map<int, ProgressToken> tokenIdMap;  // 桌面科技标记
        std::map<int, std::string> oppCardIdMap;  // 对手已建成卡牌 (用于摧毁)
        std::map<int, std::string> discardIdMap;  // 弃牌堆卡牌 (用于陵墓)
        std::map<int, ProgressToken> boxTokenIdMap; // 盒子里的标记 (用于图书馆)
        std::vector<std::string> draftWonderIds;  // 轮抽

        void clear();
    };

}

#endif // SEVEN_WONDERS_DUEL_RENDERCONTEXT_H