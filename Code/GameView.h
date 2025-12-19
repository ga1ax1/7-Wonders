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

        void setLastError(const std::string& msg);
        void clearLastError();
		void printMessage(const std::string& msg);

		// 获取用户输入 (核心交互循环)
		Action promptHumanAction(const GameModel& model, GameState state);

        // [Updated] 暴露渲染接口供 AI 回合使用，增加 state 参数
        void renderGameForAI(const GameModel& model, GameState state);

	private:
        // --- 内部状态 ---
        std::string m_lastError;

        // 渲染上下文：用于将显示的短ID (1, 2...) 映射回字符串 ID
        struct RenderContext {
            std::map<int, std::string> cardIdMap;     // 金字塔卡牌
            std::map<int, std::string> wonderIdMap;   // 奇迹
            std::map<int, ProgressToken> tokenIdMap;  // 桌面科技标记

            // 新增：特殊状态所需的映射
            std::map<int, std::string> oppCardIdMap;  // 对手已建成卡牌 (用于摧毁)
            std::map<int, std::string> discardIdMap;  // 弃牌堆卡牌 (用于陵墓)
            std::map<int, ProgressToken> boxTokenIdMap; // 盒子里的标记 (用于图书馆)

            std::vector<std::string> draftWonderIds;  // 轮抽
        } ctx;

		// --- 渲染组件 ---

        // 主渲染入口
		void renderGame(const GameModel& model, GameState state);

        // 分阶段/状态渲染
        void renderDraftPhase(const GameModel& model);
        void renderTokenSelection(const GameModel& model, bool fromBox);
        void renderDestructionPhase(const GameModel& model);
        void renderDiscardBuildPhase(const GameModel& model);
        void renderStartPlayerSelect(const GameModel& model);

        // 基础绘图
		void printLine(char c = '-', int width = 80);
        void printCentered(const std::string& text, int width = 80);

        // 格式化辅助
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
        void renderProgressTokens(const std::vector<ProgressToken>& tokens, bool isBoxContext = false);

        // Dashboard
		void renderPlayerDashboard(const Player& p, bool isCurrent, const Player& opp, int& wonderCounter, bool targetMode = false);

        void renderPyramid(const GameModel& model);
        void renderActionLog(const std::vector<std::string>& log);
        void renderCommandHelp(GameState state);
        void renderErrorMessage();

        // 详情页
		void renderPlayerDetailFull(const Player& p, const Player& opp);
		void renderCardDetail(const Card& c);
		void renderWonderDetail(const Wonder& w);
		void renderTokenDetail(ProgressToken t);
        void renderDiscardPile(const std::vector<Card*>& pile);
        void renderFullLog(const std::vector<std::string>& log);

        // 解析辅助
        int parseId(const std::string& input, char prefix);
	};

}

#endif // SEVEN_WONDERS_DUEL_GAMEVIEW_H