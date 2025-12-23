#include "GameView.h"
#include "ScoringManager.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <sstream>
#include <limits>
#include <algorithm>
#include <map>
#include <cmath>

namespace SevenWondersDuel {

    // ========================================================== 
    //  基础工具
    // ========================================================== 

    void GameView::clearScreen() { std::cout << "\033[2J\033[1;1H"; }
    void GameView::printLine(char c, int width) { std::cout << std::string(width, c) << "\n"; }

    void GameView::printCentered(const std::string& text, int width) {
        int len = 0; bool inEsc = false;
        for(char c : text) { if(c=='\033') inEsc=true; if(!inEsc) len++; if(inEsc && c=='m') inEsc=false; }
        int padding = (width - len) / 2;
        if (padding > 0) std::cout << std::string(padding, ' ');
        std::cout << text << "\n";
    }

    void GameView::printMessage(const std::string& msg) { std::cout << "\033[96m[INFO] " << msg << "\033[0m\n"; }

    // ========================================================== 
    //  颜色与文本
    // ========================================================== 

    std::string GameView::getCardColorCode(CardType t) {
        switch(t) {
            case CardType::RAW_MATERIAL: return "\033[33m";
            case CardType::MANUFACTURED: return "\033[90m";
            case CardType::CIVILIAN: return "\033[34m";
            case CardType::SCIENTIFIC: return "\033[32m";
            case CardType::COMMERCIAL: return "\033[93m";
            case CardType::MILITARY: return "\033[31m";
            case CardType::GUILD: return "\033[35m";
            case CardType::WONDER: return "\033[36m";
            default: return "\033[0m";
        }
    }

    std::string GameView::getResetColor() { return "\033[0m"; }

    std::string GameView::getTypeStr(CardType t) {
        std::string name;
        switch(t) {
            case CardType::RAW_MATERIAL: name = "Brown"; break;
            case CardType::MANUFACTURED: name = "Grey"; break;
            case CardType::CIVILIAN: name = "Blue"; break;
            case CardType::SCIENTIFIC: name = "Green"; break;
            case CardType::COMMERCIAL: name = "Yellow"; break;
            case CardType::MILITARY: name = "Red"; break;
            case CardType::GUILD: name = "Guild"; break;
            default: name = "Unknown";
        }
        return getCardColorCode(t) + name + getResetColor();
    }

    std::string GameView::getTokenName(ProgressToken t) {
        switch(t) {
            case ProgressToken::AGRICULTURE: return "Agriculture";
            case ProgressToken::URBANISM: return "Urbanism";
            case ProgressToken::STRATEGY: return "Strategy";
            case ProgressToken::THEOLOGY: return "Theology";
            case ProgressToken::ECONOMY: return "Economy";
            case ProgressToken::MASONRY: return "Masonry";
            case ProgressToken::ARCHITECTURE: return "Architecture";
            case ProgressToken::LAW: return "Law";
            case ProgressToken::MATHEMATICS: return "Mathematics";
            case ProgressToken::PHILOSOPHY: return "Philosophy";
            default: return "Unknown";
        }
    }

    std::string GameView::resourceName(ResourceType r) {
        switch(r) {
            case ResourceType::WOOD: return "Wood"; case ResourceType::STONE: return "Stone";
            case ResourceType::CLAY: return "Clay"; case ResourceType::PAPER: return "Paper";
            case ResourceType::GLASS: return "Glass";
        } return "?";
    }

    std::string GameView::formatCost(const ResourceCost& cost) {
        std::stringstream ss;
        if(cost.getCoins() > 0) ss << "$" << cost.getCoins() << " ";
        for(auto [r,v]: cost.getResources()) { if(v>0) ss << v << resourceName(r).substr(0,1) << " "; }
        std::string s = ss.str(); return s.empty() ? "Free" : s;
    }

    std::string GameView::formatResourcesCompact(const Player& p) {
        std::stringstream ss;
        ss << "W:" << p.getFixedResources().at(ResourceType::WOOD) << " ";
        ss << "C:" << p.getFixedResources().at(ResourceType::CLAY) << " ";
        ss << "S:" << p.getFixedResources().at(ResourceType::STONE) << " ";
        ss << "G:" << p.getFixedResources().at(ResourceType::GLASS) << " ";
        ss << "P:" << p.getFixedResources().at(ResourceType::PAPER);

        if (!p.getChoiceResources().empty()) {
            ss << " \033[93m+";
            for (const auto& choices : p.getChoiceResources()) {
                ss << "(";
                for (size_t i = 0; i < choices.size(); ++i) {
                    if (i > 0) ss << "/";
                    ss << resourceName(choices[i]).substr(0,1);
                }
                ss << ")";
            }
            ss << "\033[0m";
        }
        return ss.str();
    }

    std::string formatScienceSymbols(const Player& p) {
        std::stringstream ss;
        bool hasAny = false;
        for (auto const& [sym, count] : p.getScienceSymbols()) {
            if (sym == ScienceSymbol::NONE || count == 0) continue;
            hasAny = true;
            std::string sName;
            switch(sym) {
                case ScienceSymbol::GLOBE: sName="Globe"; break;
                case ScienceSymbol::TABLET: sName="Tablet"; break;
                case ScienceSymbol::MORTAR: sName="Mortar"; break;
                case ScienceSymbol::COMPASS: sName="Compass"; break;
                case ScienceSymbol::WHEEL: sName="Wheel"; break;
                case ScienceSymbol::QUILL: sName="Quill"; break;
                case ScienceSymbol::LAW: sName="Law"; break;
                default: sName="?"; break;
            }
            ss << "\033[32m[" << sName << "]\033[0m ";
        }
        return ss.str();
    }

    // ========================================================== 
    //  主菜单渲染
    // ========================================================== 

    void GameView::renderMainMenu() {
        clearScreen();
        printLine('=', 80);
        printCentered("\033[1;33m7   W O N D E R S    D U E L\033[0m");
        printLine('=', 80);
        printCentered("Please Select Game Mode:");
        std::cout << "\n";
        std::string indent(28, ' ');
        std::cout << indent << "[1] Human vs Human\n";
        std::cout << indent << "[2] Human vs AI (Recommended)\n";
        std::cout << indent << "[3] AI vs AI (Watch Mode)\n";
        std::cout << indent << "[4] Quit Game\n";
        printLine('=', 80);
        std::cout << "  Input > ";
    }

    // ========================================================== 
    //  子模块渲染
    // ========================================================== 

    void GameView::renderMilitaryTrack(const Board& board) {
        int pos = board.getMilitaryTrack().getPosition();
        std::stringstream ss;
        ss << "        P1  ";
        for (int i = -9; i <= 9; ++i) {
            if (i == pos) ss << "\033[1;31m@\033[0m ";
            else if (i == 0) ss << "| ";
            else ss << "- ";
        }
        ss << " P2";
        std::cout << ss.str() << "\n";
        
        const bool* tokens = board.getMilitaryTrack().getLootTokens();

        std::cout << "            "
                  << (tokens[1] ? "[$ 5] " : "      ")
                  << (tokens[0] ? "[$ 2]" : "     ")
                  << "               "
                  << (tokens[2] ? "[$ 2] " : "      ")
                  << (tokens[3] ? "[$ 5]" : "     ") << "\n";
    }

    void GameView::renderProgressTokens(const std::vector<ProgressToken>& tokens, RenderContext& ctx, bool isBoxContext) {
        if (tokens.empty()) { std::cout << "[TOKENS] (Empty)\n"; return; }

        std::cout << (isBoxContext ? "[BOX TOKENS] " : "[TOKENS] ");
        for (size_t i = 0; i < tokens.size(); ++i) {
            int displayId = i + 1;
            if (isBoxContext) ctx.boxTokenIdMap[displayId] = tokens[i];
            else ctx.tokenIdMap[displayId] = tokens[i];

            std::cout << "\033[32m[S" << displayId << "]" << getTokenName(tokens[i]) << "\033[0m  ";
        }
        std::cout << "\n";
    }

    void GameView::renderHeader(const GameModel& model) {
        std::stringstream ss;
        ss << "[ 7 WONDERS DUEL - AGE " << model.getCurrentAge() << " ]";
        printLine('=');
        printCentered("\033[1;37m" + ss.str() + "\033[0m");
        renderMilitaryTrack(*model.getBoard());
    }

    void GameView::renderPlayerDashboard(const Player& p, bool isCurrent, const Player& opp, int& wonderCounter, const Board& board, RenderContext& ctx, bool targetMode) {
        std::string nameTag = isCurrent ? ("\033[1;36m[" + p.getName() + "]\033[0m") : ("[" + p.getName() + "]");
        if (targetMode) nameTag = "\033[1;31m[TARGET: " + p.getName() + "]\033[0m";

        int displayVP = ScoringManager::calculateScore(p, opp, board) - (p.getCoins()/3);

        std::cout << nameTag
                  << " Coin:\033[33m" << p.getCoins() << "\033[0m"
                  << " VP:\033[36m" << displayVP << "\033[0m "
                  << formatResourcesCompact(p) << "\n";

        std::string scienceStr = formatScienceSymbols(p);
        if (!scienceStr.empty()) {
            std::cout << "Science: " << scienceStr << "\n";
        }

        std::cout << "Wonder: ";
        for(auto w : p.getBuiltWonders()) {
            int currentId = wonderCounter++;
            ctx.wonderIdMap[currentId] = w->getId();
            std::cout << "\033[32m[W" << currentId << "][X]" << w->getName() << "\033[0m  ";
        }
        for(auto w : p.getUnbuiltWonders()) {
            int currentId = wonderCounter++;
            ctx.wonderIdMap[currentId] = w->getId();
            std::cout << "[W" << currentId << "][ ]" << w->getName() << "  ";
        }
        std::cout << "\n";

        if (targetMode) {
            ctx.oppCardIdMap.clear();
            std::cout << "Built Cards (Select to Destroy): \n";
            int idx = 1;
            for(auto c : p.getBuiltCards()) {
                std::string idStr = "T" + std::to_string(idx);
                ctx.oppCardIdMap[idx] = c->getId();

                std::cout << "  [" << idStr << "] " << getTypeStr(c->getType()) << " " << c->getName();
                if (idx % 3 == 0) std::cout << "\n";
                idx++;
            }
            std::cout << "\n";
        }

        printLine('-');
    }

    void GameView::renderPyramid(const GameModel& model, RenderContext& ctx) {
        std::cout << "           PYRAMID (" << model.getRemainingCardCount() << ") | DISCARD (" << model.getBoard()->getDiscardPile().size() << ")\n";

        const auto& slots = model.getBoard()->getCardStructure().getSlots();
        if (slots.empty()) return;

        std::map<int, std::vector<const CardSlot*>> rows;
        int maxRow = 0;
        for (const auto& slot : slots) {
            rows[slot.getRow()].push_back(&slot);
            if (slot.getRow() > maxRow) maxRow = slot.getRow();
        }

        for (int r = 0; r <= maxRow; ++r) {
            const auto& rowSlots = rows[r];
            if (rowSlots.empty()) continue;

            int cardWidth = 11;
            int rowLen = rowSlots.size() * cardWidth;

            bool isAge3SplitRow = (model.getCurrentAge() == 3 && r == 3);
            if (isAge3SplitRow) {
                rowLen += cardWidth;
            }

            int padding = (80 - rowLen) / 2;
            std::cout << std::string(std::max(0, padding), ' ');

            for (size_t i = 0; i < rowSlots.size(); ++i) {
                const auto* slot = rowSlots[i];
                int absIndex = (&(*slot) - &slots[0]) + 1;

                if (slot->isRemoved()) std::cout << "           ";
                else if (!slot->isFaceUp()) std::cout << " [\033[90m ? ? ? \033[0m] ";
                else {
                    Card* c = slot->getCardPtr();
                    ctx.cardIdMap[absIndex] = c->getId();
                    std::string label = " C" + std::to_string(absIndex) + " ";
                    while(label.length() < 7) label += " ";
                    std::cout << " [" << getCardColorCode(c->getType()) << label << getResetColor() << "] ";
                }

                if (isAge3SplitRow && i == 0) {
                    std::cout << "           ";
                }
            }
            std::cout << "\n";
        }
        printLine('-');
    }

    void GameView::renderActionLog(const std::vector<std::string>& log) {
        std::cout << " [LAST ACTION]\n";
        size_t count = std::min((size_t)2, log.size());
        for (size_t i = 0; i < count; ++i) std::cout << " > " << log[log.size() - count + i] << "\n";
        printLine('=');
    }

    void GameView::renderErrorMessage(const std::string& lastError) {
        if (!lastError.empty()) std::cout << "\033[1;31m [!] " << lastError << "\033[0m\n";
    }

    void GameView::renderCommandHelp(GameState state) {
        std::cout << " [CMD] ";
        switch (state) {
            case GameState::WONDER_DRAFT_PHASE_1:
            case GameState::WONDER_DRAFT_PHASE_2:
                std::cout << "pick <ID>\n";
                std::cout << "       ";
                std::cout << "detail <1/2>\n";
                break;
            case GameState::WAITING_FOR_TOKEN_SELECTION_PAIR:
            case GameState::WAITING_FOR_TOKEN_SELECTION_LIB:
                std::cout << "select <ID> (e.g. select S1)\n";
                std::cout << "       ";
                std::cout << "info <ID>\n";
                break;
            case GameState::WAITING_FOR_DESTRUCTION:
                std::cout << "destroy <ID>\n";
                break;
            case GameState::WAITING_FOR_DISCARD_BUILD:
                std::cout << "resurrect <ID>\n";
                break;
            case GameState::WAITING_FOR_START_PLAYER_SELECTION:
                std::cout << "choose me, choose opponent\n";
                break;
            default:
                std::cout << "build/discard <CID>\n";
                std::cout << "       ";
                std::cout << "wonder <CID> <WID>\n";
                std::cout << "       ";
                std::cout << "detail <1/2>\n";
                std::cout << "       ";
                std::cout << "pile\n";
                std::cout << "       ";
                std::cout << "info <ID>\n";
                break;
        }
    }

    // ========================================================== 
    //  特殊状态渲染
    // ========================================================== 

    void GameView::renderDraftPhase(const GameModel& model, RenderContext& ctx, const std::string& lastError) {
        clearScreen();
        printLine('='); printCentered("\033[1;36mWONDER DRAFT\033[0m"); printLine('=');
        const Player* p = model.getCurrentPlayer();
        std::cout << "  \033[1;33m[" << p->getName() << "]\033[0m Choose a Wonder:\n";

        ctx.draftWonderIds.clear();
        int idx = 1;
        for (const auto& w : model.getDraftPool()) {
            ctx.draftWonderIds.push_back(w->getId());
            std::cout << "  [" << idx << "] \033[1;37m" << std::left << std::setw(20) << w->getName() << "\033[0m";
            std::cout << " Cost: " << formatCost(w->getCost()) << " ";
            std::cout << " Eff: "; for(auto& eff : w->getEffects()) std::cout << eff->getDescription() << " ";
            std::cout << "\n";
            idx++;
        }
        printLine('-');
        renderActionLog(model.getGameLog());
        renderErrorMessage(lastError);
        renderCommandHelp(model.getCurrentPlayerIndex() == 0 ? GameState::WONDER_DRAFT_PHASE_1 : GameState::WONDER_DRAFT_PHASE_1);
    }

    void GameView::renderTokenSelection(const GameModel& model, bool fromBox, RenderContext& ctx, const std::string& lastError) {
        clearScreen();
        printLine('='); printCentered("\033[1;36mSELECT PROGRESS TOKEN\033[0m"); printLine('=');

        if (fromBox) {
            std::cout << "  Source: \033[33mGame Box (Library Effect)\033[0m\n";
            renderProgressTokens(model.getBoard()->getBoxProgressTokens(), ctx, true); // true = box context
        } else {
            std::cout << "  Source: \033[33mBoard (Science Pair)\033[0m\n";
            renderProgressTokens(model.getBoard()->getAvailableProgressTokens(), ctx, false);
        }
        printLine('-');

        renderActionLog(model.getGameLog());
        renderErrorMessage(lastError);
        renderCommandHelp(fromBox ? GameState::WAITING_FOR_TOKEN_SELECTION_LIB : GameState::WAITING_FOR_TOKEN_SELECTION_PAIR);
    }

    void GameView::renderDestructionPhase(const GameModel& model, RenderContext& ctx, const std::string& lastError) {
        clearScreen();
        printLine('='); printCentered("\033[1;31mDESTROY OPPONENT CARD\033[0m"); printLine('=');

        int wCounter = 1;
        renderPlayerDashboard(*model.getOpponent(), false, *model.getCurrentPlayer(), wCounter, *model.getBoard(), ctx, true);

        renderErrorMessage(lastError);
        renderCommandHelp(GameState::WAITING_FOR_DESTRUCTION);
    }

    void GameView::renderDiscardBuildPhase(const GameModel& model, RenderContext& ctx, const std::string& lastError) {
        clearScreen();
        printLine('='); printCentered("\033[1;35mMAUSOLEUM: RESURRECT CARD\033[0m"); printLine('=');

        const auto& pile = model.getBoard()->getDiscardPile();
        ctx.discardIdMap.clear();
        int idx = 1;

        if (pile.empty()) std::cout << "  (Discard pile is empty)\n";

        for(auto c : pile) {
            ctx.discardIdMap[idx] = c->getId();
            std::cout << "  [D" << idx++ << "] " << c->getName() << " (" << getTypeStr(c->getType()) << ")\n";
        }
        printLine('-');
        renderErrorMessage(lastError);
        renderCommandHelp(GameState::WAITING_FOR_DISCARD_BUILD);
    }

    void GameView::renderStartPlayerSelect(const GameModel& model, const std::string& lastError) {
        clearScreen();
        printLine('='); printCentered("CHOOSE STARTING PLAYER"); printLine('=');
        std::cout << "  \033[1;33m[" << model.getCurrentPlayer()->getName() << "]\033[0m decides who starts the next Age.\n";
        std::cout << "  (Decision based on military strength or last played turn)\n\n";
        std::cout << "  Available Commands:\n";
        std::cout << "  > \033[32mchoose me\033[0m        (You take the first turn)\n";
        std::cout << "  > \033[32mchoose opponent\033[0m  (" << model.getOpponent()->getName() << " takes the first turn)\n";
        printLine('-');
        renderErrorMessage(lastError);
        std::cout << " [CMD] Input command directly above.\n";
    }

    // ========================================================== 
    //  主入口：renderGame 统一分发
    // ========================================================== 

    void GameView::renderGame(const GameModel& model, GameState state, RenderContext& ctx, const std::string& lastError) {
        int wonderCounter = 1;

        switch (state) {
            case GameState::WONDER_DRAFT_PHASE_1:
            case GameState::WONDER_DRAFT_PHASE_2:
                renderDraftPhase(model, ctx, lastError);
                break;
            case GameState::WAITING_FOR_TOKEN_SELECTION_PAIR:
                renderTokenSelection(model, false, ctx, lastError);
                break;
            case GameState::WAITING_FOR_TOKEN_SELECTION_LIB:
                renderTokenSelection(model, true, ctx, lastError);
                break;
            case GameState::WAITING_FOR_DESTRUCTION:
                renderDestructionPhase(model, ctx, lastError);
                break;
            case GameState::WAITING_FOR_DISCARD_BUILD:
                renderDiscardBuildPhase(model, ctx, lastError);
                break;
            case GameState::WAITING_FOR_START_PLAYER_SELECTION:
                renderStartPlayerSelect(model, lastError);
                break;
            default: // AGE_PLAY_PHASE or GAME_OVER
                clearScreen();
                renderHeader(model);
                renderProgressTokens(model.getBoard()->getAvailableProgressTokens(), ctx, false);
                printLine('-');
                renderPlayerDashboard(*model.getPlayers()[0], model.getCurrentPlayerIndex() == 0, *model.getPlayers()[1], wonderCounter, *model.getBoard(), ctx);
                renderPyramid(model, ctx);
                renderPlayerDashboard(*model.getPlayers()[1], model.getCurrentPlayerIndex() == 1, *model.getPlayers()[0], wonderCounter, *model.getBoard(), ctx);
                renderActionLog(model.getGameLog());
                renderErrorMessage(lastError);
                renderCommandHelp(state);
                break;
        }
    }

    void GameView::renderGameForAI(const GameModel& model, GameState state) {
        RenderContext dummy;
        renderGame(model, state, dummy, "");
    }

    // ========================================================== 
    //  详情页 (View Only Screens)
    // ========================================================== 

    void GameView::renderPlayerDetailFull(const Player& p, const Player& opp, const Board& board) {
        clearScreen();
        printLine('='); printCentered("DETAIL: " + p.getName());

        int discardValue = Config::BASE_DISCARD_GAIN + p.getCardCount(CardType::COMMERCIAL);

        std::cout << " [1] BASIC: Coins " << p.getCoins() << " | VP " << ScoringManager::calculateScore(p, opp, board) << "\n";
        std::cout << "     \033[33mDiscard Value: " << discardValue << " coins\033[0m\n";

        std::cout << " [2] RESOURCES: " << formatResourcesCompact(p) << "\n";
        std::cout << "     Buy Costs (W/C/S/G/P): ";
        std::vector<ResourceType> types = {ResourceType::WOOD, ResourceType::CLAY, ResourceType::STONE, ResourceType::GLASS, ResourceType::PAPER};
        for (auto t : types) std::cout << p.getTradingPrice(t, opp) << "$ ";

        std::cout << "\n [3] SCIENCE: ";
        for(auto const& [s, c] : p.getScienceSymbols()) { if(c>0 && s!=ScienceSymbol::NONE) std::cout << "[" << (int)s << "]x" << c << " "; } // Corrected: Use const auto& for map iteration
        std::cout << "\n [4] WONDERS:\n";
        for(auto w : p.getBuiltWonders()) std::cout << "     [Built] " << w->getName() << "\n";
        for(auto w : p.getUnbuiltWonders()) std::cout << "     [Plan ] " << w->getName() << "\n";
        printLine('='); std::cout << " (Press Enter)"; std::cin.get();
    }

    void GameView::renderCardDetail(const Card& c) {
        clearScreen(); printLine('='); printCentered("INFO: " + c.getName());
        std::cout << "  Type: " << getTypeStr(c.getType()) << " | Cost: " << formatCost(c.getCost()) << "\n";
        std::cout << "  Eff: "; for(auto& eff : c.getEffects()) std::cout << eff->getDescription() << " ";
        std::cout << "\n";

        if (!c.getChainTag().empty()) std::cout << "  Provides Chain: \033[97m" << c.getChainTag() << "\033[0m\n";
        if (!c.getRequiresChainTag().empty()) std::cout << "  Requires Chain: \033[97m" << c.getRequiresChainTag() << "\033[0m\n";

        printLine('='); std::cout << " (Press Enter)"; std::cin.get();
    }

    void GameView::renderWonderDetail(const Wonder& w) {
        clearScreen(); printLine('='); printCentered("INFO: " + w.getName());
        std::cout << "  Cost: " << formatCost(w.getCost()) << "\n";
        std::cout << "  Eff: "; for(auto& eff : w.getEffects()) std::cout << eff->getDescription() << " ";
        std::cout << "\n"; printLine('='); std::cout << " (Press Enter)"; std::cin.get();
    }

    void GameView::renderTokenDetail(ProgressToken t) {
        clearScreen(); printLine('='); printCentered("TOKEN: " + getTokenName(t));
        std::cout << "  (See Manual)\n";
        printLine('='); std::cout << " (Press Enter)"; std::cin.get();
    }

    void GameView::renderDiscardPile(const std::vector<Card*>& pile) {
        clearScreen(); printLine('='); printCentered("DISCARD PILE (" + std::to_string(pile.size()) + ")");
        int idx = 1; for(auto c : pile) std::cout << "  [D" << idx++ << "] " << c->getName() << " (" << getTypeStr(c->getType()) << ")\n";
        printLine('='); std::cout << " (Press Enter)"; std::cin.get();
    }

    void GameView::renderFullLog(const std::vector<std::string>& log) {
        clearScreen(); printLine('='); printCentered("GAME LOG");
        for(const auto& l : log) std::cout << " " << l << "\n";
        printLine('='); std::cout << " (Press Enter)"; std::cin.get();
    }
}
