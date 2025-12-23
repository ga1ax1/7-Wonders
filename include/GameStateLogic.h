//
// Created by choyichi on 2025/12/23.
//

#ifndef SEVEN_WONDERS_DUEL_GAMESTATELOGIC_H
#define SEVEN_WONDERS_DUEL_GAMESTATELOGIC_H

#include "Global.h"
#include <memory>

namespace SevenWondersDuel {

    class GameController; // Forward declaration

    class IGameStateLogic {
    public:
        virtual ~IGameStateLogic() = default;
        
        virtual void onEnter(GameController& controller);
        virtual ActionResult validate(const Action& action, GameController& controller) = 0;
    };

    // 1. 奇迹轮抽状态
    class WonderDraftState : public IGameStateLogic {
    public:
        ActionResult validate(const Action& action, GameController& controller) override;
        void onEnter(GameController& controller) override;
    };

    // 2. 时代主流程状态
    class AgePlayState : public IGameStateLogic {
    public:
        ActionResult validate(const Action& action, GameController& controller) override;
    };

    // 3. 等待选择科技标记 (配对或图书馆)
    class TokenSelectionState : public IGameStateLogic {
    public:
        ActionResult validate(const Action& action, GameController& controller) override;
    };

    // 4. 等待摧毁卡牌
    class DestructionState : public IGameStateLogic {
    public:
        ActionResult validate(const Action& action, GameController& controller) override;
    };

    // 5. 等待弃牌堆复活
    class DiscardBuildState : public IGameStateLogic {
    public:
        ActionResult validate(const Action& action, GameController& controller) override;
    };

    // 6. 等待选择先手
    class StartPlayerSelectionState : public IGameStateLogic {
    public:
        ActionResult validate(const Action& action, GameController& controller) override;
    };
    
    // 7. 游戏结束
    class GameOverState : public IGameStateLogic {
    public:
        ActionResult validate(const Action& action, GameController& controller) override;
    };


}

#endif // SEVEN_WONDERS_DUEL_GAMESTATELOGIC_H
