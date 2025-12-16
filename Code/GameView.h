//
// Created by choyichi on 2025/12/16.
//

#ifndef SEVEN_WONDERS_DUEL_GAMEVIEW_H
#define SEVEN_WONDERS_DUEL_GAMEVIEW_H

#include "Global.h"
#include "GameController.h"
#include <string>

namespace SevenWondersDuel {

	class GameView {
	public:
		GameView() = default;

		// 清屏
		void clearScreen();

		// 渲染主菜单 (选择AI/人类)
		void renderMainMenu();

		// 渲染整个游戏界面
		void renderGame(const GameModel& model);

		// 显示当前谁的回合
		void printTurnInfo(const Player* player);

		// 打印普通消息或错误
		void printMessage(const std::string& msg);
		void printError(const std::string& msg);

		// 获取用户输入 (人类玩家使用)
		// 返回构造好的 Action，如果不合法或用户取消，ActionType 可能为 NONE (-1) (需调用者处理)
		Action promptHumanAction(const GameModel& model);

	private:
		// 内部辅助渲染函数
		void renderHeader(const std::string& text);

		std::string getTokenName(ProgressToken t);

		void renderPlayer(const Player& p, bool isCurrent);
		void renderBoard(const GameModel& model);
		std::string getCardColorCode(CardType t);
		std::string resourceToString(ResourceType r);
		void printLine(char c = '-', int width = 80);

		void renderPlayerDetailFull(const Player& p, const Player& opp);

		void renderCardDetail(const Card& c);
		void renderWonderDetail(const Wonder& w);
		void renderTokenDetail(ProgressToken t);
	};

}

#endif // SEVEN_WONDERS_DUEL_GAMEVIEW_H