//
// Created by choyichi on 2025/12/16.
//

#ifndef SEVEN_WONDERS_DUEL_GAMEVIEW_H
#define SEVEN_WONDERS_DUEL_GAMEVIEW_H

#include "Global.h"
#include "GameController.h"
#include "RenderContext.h"
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

		void printMessage(const std::string& msg);

        // [Updated] 暴露渲染接口供使用，增加 ctx 和 lastError 参数
        void renderGame(const GameModel& model, GameState state, RenderContext& ctx, const std::string& lastError);
        void renderGameForAI(const GameModel& model, GameState state);

        // 详情页 (Public because InputManager needs them)
		void renderPlayerDetailFull(const Player& p, const Player& opp, const Board& board);
		void renderCardDetail(const Card& c);
		void renderWonderDetail(const Wonder& w);
		void renderTokenDetail(ProgressToken t);
        void renderDiscardPile(const std::vector<Card*>& pile);
        void renderFullLog(const std::vector<std::string>& log);

	private:
		// --- 渲染组件 ---

        // 分阶段/状态渲染
        void renderDraftPhase(const GameModel& model, RenderContext& ctx, const std::string& lastError);
        void renderTokenSelection(const GameModel& model, bool fromBox, RenderContext& ctx, const std::string& lastError);
        void renderDestructionPhase(const GameModel& model, RenderContext& ctx, const std::string& lastError);
        void renderDiscardBuildPhase(const GameModel& model, RenderContext& ctx, const std::string& lastError);
        void renderStartPlayerSelect(const GameModel& model, const std::string& lastError);

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
        void renderProgressTokens(const std::vector<ProgressToken>& tokens, RenderContext& ctx, bool isBoxContext = false);

        // Dashboard
		void renderPlayerDashboard(const Player& p, bool isCurrent, const Player& opp, int& wonderCounter, const Board& board, RenderContext& ctx, bool targetMode = false);

        void renderPyramid(const GameModel& model, RenderContext& ctx);
        void renderActionLog(const std::vector<std::string>& log);
        void renderCommandHelp(GameState state);
        void renderErrorMessage(const std::string& lastError);
	};

}

#endif // SEVEN_WONDERS_DUEL_GAMEVIEW_H
