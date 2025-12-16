//
// Created by choyichi on 2025/12/16.
//

#ifndef SEVEN_WONDERS_DUEL_AGENT_H
#define SEVEN_WONDERS_DUEL_AGENT_H

#include "GameController.h"
#include "GameView.h"

namespace SevenWondersDuel {

	class IPlayerAgent {
	public:
		virtual ~IPlayerAgent() = default;
		// 核心方法：给定当前模型，返回一个动作
		virtual Action decideAction(GameController& controller, GameView& view) = 0;

		// 用于标识
		virtual bool isHuman() const { return false; }
	};

	// 人类玩家：通过 View 获取输入
	class HumanAgent : public IPlayerAgent {
	public:
		Action decideAction(GameController& controller, GameView& view) override;
		bool isHuman() const override { return true; }
	};

	// AI玩家：简单的随机策略
	class RandomAIAgent : public IPlayerAgent {
	public:
		Action decideAction(GameController& controller, GameView& view) override;
	};

}

#endif // SEVEN_WONDERS_DUEL_AGENT_H