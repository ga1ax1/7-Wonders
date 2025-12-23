#ifndef SEVEN_WONDERS_DUEL_INPUTMANAGER_H
#define SEVEN_WONDERS_DUEL_INPUTMANAGER_H

#include "Global.h"
#include "GameController.h"
#include "RenderContext.h"
#include <string>
#include <vector>
#include <map>

namespace SevenWondersDuel {

    class GameView;

    class InputManager {
    public:
        InputManager() = default;

        // 获取用户输入 (核心交互循环)
        Action promptHumanAction(GameView& view, const GameModel& model, GameState state);

        void setLastError(const std::string& msg) { m_lastError = msg; }
        void clearLastError() { m_lastError = ""; }
        const std::string& getLastError() const { return m_lastError; }

    private:
        int parseId(const std::string& input, char prefix);

        RenderContext m_ctx;
        std::string m_lastError;
    };

}

#endif // SEVEN_WONDERS_DUEL_INPUTMANAGER_H
