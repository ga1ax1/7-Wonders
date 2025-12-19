//
// Created by choyichi on 2025/12/16.
//

#ifndef SEVEN_WONDERS_DUEL_GAMEVIEW_H
#define SEVEN_WONDERS_DUEL_GAMEVIEW_H

#include "Global.h"
#include "GameController.h"
#include <string>
#include <vector>

namespace SevenWondersDuel {

	class GameView {
	public:
		GameView() = default;

		// --- 公共接口 ---

		// 清屏
		void clearScreen();

		// 渲染主菜单 (Start Screen)
		void renderMainMenu();

		// 渲染整个游戏主界面 (Dashboard)
		void renderGame(const GameModel& model);

		// 打印普通消息或错误 (带颜色)
		void printMessage(const std::string& msg);
		void printError(const std::string& msg);

        // 打印当前回合提示
        void printTurnInfo(const Player* player);

		// 获取用户输入 (人类玩家使用) - 包含内部交互循环
		Action promptHumanAction(const GameModel& model, GameState state);

	private:
		// --- 内部渲染组件 ---

		// 辅助：打印分割线
		void printLine(char c = '-', int width = 80);
        // 辅助：打印居中文本
        void printCentered(const std::string& text, int width = 80);

        // 颜色代码获取
		std::string getCardColorCode(CardType t);
        std::string getResetColor();
        std::string getTypeStr(CardType t); // 带颜色的类型名

		// 文本转换
		std::string getTokenName(ProgressToken t);
        std::string getTokenShortName(ProgressToken t); // 短名 [S1]Law
		std::string resourceName(ResourceType r);
        std::string formatCost(const ResourceCost& cost);
        std::string formatResourcesCompact(const Player& p);

        // 模块渲染
		void renderHeader(const GameModel& model);
		void renderMilitaryTrack(const Board& board);
        void renderProgressTokens(const std::vector<ProgressToken>& tokens);
		void renderPlayerDashboard(const Player& p, bool isCurrent, const Player& opp);
        void renderPyramid(const GameModel& model);
        void renderActionLog(const std::vector<std::string>& log);
        void renderCommandHelp();

        // 辅助页面
		void renderPlayerDetailFull(const Player& p, const Player& opp);
		void renderCardDetail(const Card& c);
		void renderWonderDetail(const Wonder& w);
		void renderTokenDetail(ProgressToken t);
        void renderDiscardPile(const std::vector<Card*>& pile);
        void renderFullLog(const std::vector<std::string>& log);
	};

}

#endif // SEVEN_WONDERS_DUEL_GAMEVIEW_H