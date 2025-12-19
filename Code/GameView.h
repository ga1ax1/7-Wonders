//
// Created by choyichi on 2025/12/16.
//

#ifndef SEVEN_WONDERS_DUEL_GAMEVIEW_H
#define SEVEN_WONDERS_DUEL_GAMEVIEW_H

#include "Global.h"
#include "GameController.h"
#include <string>
#include <vector>
#include <map>

namespace SevenWondersDuel {

	class GameView {
	public:
		GameView() = default;

		// --- 公共接口 ---

		void clearScreen();
		void renderMainMenu();

        // 设置错误信息（将在下一次 renderGame 中显示）
        void setLastError(const std::string& msg);
        void clearLastError();

        // 打印普通消息
		void printMessage(const std::string& msg);
        // (已弃用，功能合并入 Dashboard) void printTurnInfo(const Player* player);

		// 获取用户输入 (人类玩家使用) - 包含内部交互循环
		Action promptHumanAction(const GameModel& model, GameState state);

        // 暴露渲染接口供 AI 回合使用
        void renderGameForAI(const GameModel& model);

	private:
        // --- 内部状态 ---
        std::string m_lastError; // 存储上一条错误信息

        // ID 映射上下文
        struct RenderContext {
            std::map<int, std::string> cardIdMap;   // Display ID -> Card ID
            std::map<int, std::string> wonderIdMap; // Display ID -> Wonder ID (全局编号)
            std::map<int, ProgressToken> tokenIdMap;// Display ID -> Token
            std::vector<std::string> draftWonderIds; // Index 0-based -> Wonder ID
        } ctx;

		// --- 渲染组件 ---

		void renderGame(const GameModel& model);
        void renderDraftPhase(const GameModel& model);

        // 基础绘图
		void printLine(char c = '-', int width = 80);
        void printCentered(const std::string& text, int width = 80);

        // 文本与颜色
		std::string getCardColorCode(CardType t);
        std::string getResetColor();
        std::string getTypeStr(CardType t);
		std::string getTokenName(ProgressToken t);
        std::string resourceName(ResourceType r);
        std::string formatCost(const ResourceCost& cost);
        std::string formatResourcesCompact(const Player& p);

        // 模块渲染
		void renderHeader(const GameModel& model);
		void renderMilitaryTrack(const Board& board);
        void renderProgressTokens(const std::vector<ProgressToken>& tokens);
		// [Updated] 增加 wonderCounter 参数以实现全局编号
		void renderPlayerDashboard(const Player& p, bool isCurrent, const Player& opp, int& wonderCounter);
        void renderPyramid(const GameModel& model);
        void renderActionLog(const std::vector<std::string>& log);
        void renderCommandHelp(bool isDraft);
        void renderErrorMessage(); // 专门渲染错误区域

        // 辅助页面
		void renderPlayerDetailFull(const Player& p, const Player& opp);
		void renderCardDetail(const Card& c);
		void renderWonderDetail(const Wonder& w);
		void renderTokenDetail(ProgressToken t);
        void renderDiscardPile(const std::vector<Card*>& pile);
        void renderFullLog(const std::vector<std::string>& log);

        // 内部辅助
        int parseId(const std::string& input, char prefix);
	};

}

#endif // SEVEN_WONDERS_DUEL_GAMEVIEW_H