#include "GameView.h"
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

    int GameView::parseId(const std::string& input, char prefix) {
        if (input.empty()) return -1;
        std::string numPart = input;
        if (toupper(input[0]) == prefix && input.size() > 1) numPart = input.substr(1);
        try { return std::stoi(numPart); } catch (...) { return -1; }
    }

    void GameView::setLastError(const std::string& msg) { m_lastError = msg; }
    void GameView::clearLastError() { m_lastError = ""; }
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
        if(cost.coins > 0) ss << "$" << cost.coins << " ";
        for(auto [r,v]: cost.resources) { if(v>0) ss << v << resourceName(r).substr(0,1) << " "; }
        std::string s = ss.str(); return s.empty() ? "Free" : s;
    }

    std::string GameView::formatResourcesCompact(const Player& p) {
        std::stringstream ss;
        ss << "W:" << p.fixedResources.at(ResourceType::WOOD) << " ";
        ss << "C:" << p.fixedResources.at(ResourceType::CLAY) << " ";
        ss << "S:" << p.fixedResources.at(ResourceType::STONE) << " ";
        ss << "G:" << p.fixedResources.at(ResourceType::GLASS) << " ";
        ss << "P:" << p.fixedResources.at(ResourceType::PAPER);

        if (!p.choiceResources.empty()) {
            ss << " \033[93m+";
            for (const auto& choices : p.choiceResources) {
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
        for (auto const& [sym, count] : p.scienceSymbols) {
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
        int pos = board.militaryTrack.position;
        std::stringstream ss;
        ss << "        P1  ";
        for (int i = -9; i <= 9; ++i) {
            if (i == pos) ss << "\033[1;31m@\033[0m ";
            else if (i == 0) ss << "| ";
            else ss << "- ";
        }
        ss << " P2";
        std::cout << ss.str() << "\n";
        std::cout << "            "
                  << (board.militaryTrack.lootTokens[1] ? "[$ 5] " : "      ")
                  << (board.militaryTrack.lootTokens[0] ? "[$ 2]" : "     ")
                  << "               "
                  << (board.militaryTrack.lootTokens[2] ? "[$ 2] " : "      ")
                  << (board.militaryTrack.lootTokens[3] ? "[$ 5]" : "     ") << "\n";
    }

    void GameView::renderProgressTokens(const std::vector<ProgressToken>& tokens, bool isBoxContext) {
        if (!isBoxContext) ctx.tokenIdMap.clear();
        else ctx.boxTokenIdMap.clear();

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
        ss << "[ 7 WONDERS DUEL - AGE " << model.currentAge << " ]";
        printLine('=');
        printCentered("\033[1;37m" + ss.str() + "\033[0m");
        renderMilitaryTrack(*model.board);
        renderProgressTokens(model.board->availableProgressTokens, false);
        printLine('-');
    }

    void GameView::renderPlayerDashboard(const Player& p, bool isCurrent, const Player& opp, int& wonderCounter, bool targetMode) {
        std::string nameTag = isCurrent ? ("\033[1;36m[" + p.name + "]\033[0m") : ("[" + p.name + "]");
        if (targetMode) nameTag = "\033[1;31m[TARGET: " + p.name + "]\033[0m";

        int displayVP = p.getScore(opp) - (p.coins/3);

        std::cout << nameTag
                  << " Coin:\033[33m" << p.coins << "\033[0m"
                  << " VP:\033[36m" << displayVP << "\033[0m "
                  << formatResourcesCompact(p) << "\n";

        std::string scienceStr = formatScienceSymbols(p);
        if (!scienceStr.empty()) {
            std::cout << "Science: " << scienceStr << "\n";
        }

        std::cout << "Wonder: ";
        for(auto w : p.builtWonders) {
            int currentId = wonderCounter++;
            ctx.wonderIdMap[currentId] = w->id;
            std::cout << "\033[32m[W" << currentId << "][X]" << w->name << "\033[0m  ";
        }
        for(auto w : p.unbuiltWonders) {
            int currentId = wonderCounter++;
            ctx.wonderIdMap[currentId] = w->id;
            std::cout << "[W" << currentId << "][ ]" << w->name << "  ";
        }
        std::cout << "\n";

        if (targetMode) {
            ctx.oppCardIdMap.clear();
            std::cout << "Built Cards (Select to Destroy): \n";
            int idx = 1;
            for(auto c : p.builtCards) {
                std::string idStr = "T" + std::to_string(idx);
                ctx.oppCardIdMap[idx] = c->id;

                std::cout << "  [" << idStr << "] " << getTypeStr(c->type) << " " << c->name;
                if (idx % 3 == 0) std::cout << "\n";
                idx++;
            }
            std::cout << "\n";
        }

        printLine('-');
    }

    void GameView::renderPyramid(const GameModel& model) {
        std::cout << "           PYRAMID (" << model.getRemainingCardCount() << ") | DISCARD (" << model.board->discardPile.size() << ")\n";

        const auto& slots = model.board->cardStructure.slots;
        if (slots.empty()) return;

        ctx.cardIdMap.clear();

        std::map<int, std::vector<const CardSlot*>> rows;
        int maxRow = 0;
        for (const auto& slot : slots) {
            rows[slot.row].push_back(&slot);
            if (slot.row > maxRow) maxRow = slot.row;
        }

        for (int r = 0; r <= maxRow; ++r) {
            const auto& rowSlots = rows[r];
            if (rowSlots.empty()) continue;

            int cardWidth = 11;
            int rowLen = rowSlots.size() * cardWidth;

            bool isAge3SplitRow = (model.currentAge == 3 && r == 3);
            if (isAge3SplitRow) {
                rowLen += cardWidth;
            }

            int padding = (80 - rowLen) / 2;
            std::cout << std::string(std::max(0, padding), ' ');

            for (size_t i = 0; i < rowSlots.size(); ++i) {
                const auto* slot = rowSlots[i];
                int absIndex = (&(*slot) - &slots[0]) + 1;

                if (slot->isRemoved) std::cout << "           ";
                else if (!slot->isFaceUp) std::cout << " [\033[90m ? ? ? \033[0m] ";
                else {
                    Card* c = slot->cardPtr;
                    ctx.cardIdMap[absIndex] = c->id;
                    std::string label = " C" + std::to_string(absIndex) + " ";
                    while(label.length() < 7) label += " ";
                    std::cout << " [" << getCardColorCode(c->type) << label << getResetColor() << "] ";
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

    void GameView::renderErrorMessage() {
        if (!m_lastError.empty()) std::cout << "\033[1;31m [!] " << m_lastError << "\033[0m\n";
    }

    void GameView::renderCommandHelp(GameState state) {
        std::cout << " [CMD] ";
        switch (state) {
            case GameState::WONDER_DRAFT_PHASE_1:
            case GameState::WONDER_DRAFT_PHASE_2:
                std::cout << "pick <ID>" << std::endl;
                std::cout << "       ";
                std::cout << "detail <1/2>" << std::endl;
                break;
            case GameState::WAITING_FOR_TOKEN_SELECTION_PAIR:
            case GameState::WAITING_FOR_TOKEN_SELECTION_LIB:
                std::cout << "select <ID> (e.g. select S1)" << std::endl;
                std::cout << "       ";
                std::cout << "info <ID>" << std::endl;
                break;
            case GameState::WAITING_FOR_DESTRUCTION:
                std::cout << "destroy <ID>" << std::endl;
                std::cout << "       ";
                std::cout << "skip" <<std::endl;
                break;
            case GameState::WAITING_FOR_DISCARD_BUILD:
                std::cout << "resurrect <ID>" <<std::endl;
                break;
            case GameState::WAITING_FOR_START_PLAYER_SELECTION:
                std::cout << "choose me, choose opponent" <<std::endl;
                break;
            default:
                std::cout << "build/discard <CID>" << std::endl;
                std::cout << "       ";
                std::cout << "wonder <CID> <WID>" <<std::endl;
                std::cout << "       ";
                std::cout << "detail <1/2>" <<std::endl;
                std::cout << "       ";
                std::cout << "pile" <<std::endl;
                std::cout << "       ";
                std::cout << "info <ID>" <<std::endl;
                break;
        }
    }

    // ==========================================================
    //  特殊状态渲染
    // ==========================================================

    void GameView::renderDraftPhase(const GameModel& model) {
        clearScreen();
        printLine('='); printCentered("\033[1;36mWONDER DRAFT\033[0m"); printLine('=');
        const Player* p = model.getCurrentPlayer();
        std::cout << "  \033[1;33m[" << p->name << "]\033[0m Choose a Wonder:\n";

        ctx.draftWonderIds.clear();
        int idx = 1;
        for (const auto& w : model.draftPool) {
            ctx.draftWonderIds.push_back(w->id);
            std::cout << "  [" << idx << "] \033[1;37m" << std::left << std::setw(20) << w->name << "\033[0m";
            std::cout << " Cost: " << formatCost(w->cost) << " ";
            std::cout << " Eff: "; for(auto& eff : w->effects) std::cout << eff->getDescription() << " ";
            std::cout << "\n";
            idx++;
        }
        printLine('-');
        renderActionLog(model.gameLog);
        renderErrorMessage();
        renderCommandHelp(model.currentPlayerIndex == 0 ? GameState::WONDER_DRAFT_PHASE_1 : GameState::WONDER_DRAFT_PHASE_1);
    }

    void GameView::renderTokenSelection(const GameModel& model, bool fromBox) {
        clearScreen();
        printLine('='); printCentered("\033[1;36mSELECT PROGRESS TOKEN\033[0m"); printLine('=');

        if (fromBox) {
            std::cout << "  Source: \033[33mGame Box (Library Effect)\033[0m\n";
            renderProgressTokens(model.board->boxProgressTokens, true); // true = box context
        } else {
            std::cout << "  Source: \033[33mBoard (Science Pair)\033[0m\n";
            renderProgressTokens(model.board->availableProgressTokens, false);
        }
        printLine('-');

        // [Update] 增加日志显示，这样玩家知道为什么触发了选择
        renderActionLog(model.gameLog);

        renderErrorMessage();
        renderCommandHelp(fromBox ? GameState::WAITING_FOR_TOKEN_SELECTION_LIB : GameState::WAITING_FOR_TOKEN_SELECTION_PAIR);
    }

    void GameView::renderDestructionPhase(const GameModel& model) {
        clearScreen();
        printLine('='); printCentered("\033[1;31mDESTROY OPPONENT CARD\033[0m"); printLine('=');

        int wCounter = 1;
        renderPlayerDashboard(*model.getOpponent(), false, *model.getCurrentPlayer(), wCounter, true);

        renderErrorMessage();
        renderCommandHelp(GameState::WAITING_FOR_DESTRUCTION);
    }

    void GameView::renderDiscardBuildPhase(const GameModel& model) {
        clearScreen();
        printLine('='); printCentered("\033[1;35mMAUSOLEUM: RESURRECT CARD\033[0m"); printLine('=');

        const auto& pile = model.board->discardPile;
        ctx.discardIdMap.clear();
        int idx = 1;

        if (pile.empty()) std::cout << "  (Discard pile is empty)\n";

        for(auto c : pile) {
            ctx.discardIdMap[idx] = c->id;
            std::cout << "  [D" << idx++ << "] " << c->name << " (" << getTypeStr(c->type) << ")\n";
        }
        printLine('-');
        renderErrorMessage();
        renderCommandHelp(GameState::WAITING_FOR_DISCARD_BUILD);
    }

    void GameView::renderStartPlayerSelect(const GameModel& model) {
        clearScreen();
        printLine('='); printCentered("CHOOSE STARTING PLAYER"); printLine('=');
        std::cout << "  \033[1;33m[" << model.getCurrentPlayer()->name << "]\033[0m decides who starts the next Age.\n";
        std::cout << "  (Decision based on military strength or last played turn)\n\n";
        std::cout << "  Available Commands:\n";
        std::cout << "  > \033[32mchoose me\033[0m        (You take the first turn)\n";
        std::cout << "  > \033[32mchoose opponent\033[0m  (" << model.getOpponent()->name << " takes the first turn)\n";
        printLine('-');
        renderErrorMessage();
        // 修改 help 提示，不显示
        std::cout << " [CMD] Input command directly above.\n";
    }

    // ==========================================================
    //  主入口：renderGame 统一分发
    // ==========================================================

    void GameView::renderGame(const GameModel& model, GameState state) {
        ctx.wonderIdMap.clear();
        ctx.cardIdMap.clear();
        ctx.tokenIdMap.clear();
        ctx.boxTokenIdMap.clear();
        ctx.oppCardIdMap.clear();
        ctx.discardIdMap.clear();
        int wonderCounter = 1;

        switch (state) {
            case GameState::WONDER_DRAFT_PHASE_1:
            case GameState::WONDER_DRAFT_PHASE_2:
                renderDraftPhase(model);
                break;
            case GameState::WAITING_FOR_TOKEN_SELECTION_PAIR:
                renderTokenSelection(model, false);
                break;
            case GameState::WAITING_FOR_TOKEN_SELECTION_LIB:
                renderTokenSelection(model, true);
                break;
            case GameState::WAITING_FOR_DESTRUCTION:
                renderDestructionPhase(model);
                break;
            case GameState::WAITING_FOR_DISCARD_BUILD:
                renderDiscardBuildPhase(model);
                break;
            case GameState::WAITING_FOR_START_PLAYER_SELECTION:
                renderStartPlayerSelect(model);
                break;
            default: // AGE_PLAY_PHASE or GAME_OVER
                clearScreen();
                renderHeader(model);
                renderPlayerDashboard(*model.players[0], model.currentPlayerIndex == 0, *model.players[1], wonderCounter);
                renderPyramid(model);
                renderPlayerDashboard(*model.players[1], model.currentPlayerIndex == 1, *model.players[0], wonderCounter);
                renderActionLog(model.gameLog);
                renderErrorMessage();
                renderCommandHelp(state);
                break;
        }
    }

    void GameView::renderGameForAI(const GameModel& model, GameState state) {
        renderGame(model, state);
    }

    // ==========================================================
    //  详情页 (View Only Screens)
    // ==========================================================

    void GameView::renderPlayerDetailFull(const Player& p, const Player& opp) {
        clearScreen();
        printLine('='); printCentered("DETAIL: " + p.name);

        int discardValue = 2 + p.getCardCount(CardType::COMMERCIAL);

        std::cout << " [1] BASIC: Coins " << p.coins << " | VP " << p.getScore(opp) << "\n";
        std::cout << "     \033[33mDiscard Value: " << discardValue << " coins\033[0m\n";

        std::cout << " [2] RESOURCES: " << formatResourcesCompact(p) << "\n";
        std::cout << "     Buy Costs (W/C/S/G/P): ";
        std::vector<ResourceType> types = {ResourceType::WOOD, ResourceType::CLAY, ResourceType::STONE, ResourceType::GLASS, ResourceType::PAPER};
        for (auto t : types) std::cout << p.getTradingPrice(t, opp) << "$ ";

        std::cout << "\n [3] SCIENCE: ";
        for(auto [s, c] : p.scienceSymbols) { if(c>0 && s!=ScienceSymbol::NONE) std::cout << "[" << (int)s << "]x" << c << " "; }
        std::cout << "\n [4] WONDERS:\n";
        for(auto w : p.builtWonders) std::cout << "     [Built] " << w->name << "\n";
        for(auto w : p.unbuiltWonders) std::cout << "     [Plan ] " << w->name << "\n";
        printLine('='); std::cout << " (Press Enter)"; std::cin.get();
    }

    void GameView::renderCardDetail(const Card& c) {
        clearScreen(); printLine('='); printCentered("INFO: " + c.name);
        std::cout << "  Type: " << getTypeStr(c.type) << " | Cost: " << formatCost(c.cost) << "\n";
        std::cout << "  Eff: "; for(auto& eff : c.effects) std::cout << eff->getDescription() << " ";
        std::cout << "\n";

        if (!c.chainTag.empty()) std::cout << "  Provides Chain: \033[97m" << c.chainTag << "\033[0m\n";
        if (!c.requiresChainTag.empty()) std::cout << "  Requires Chain: \033[97m" << c.requiresChainTag << "\033[0m\n";

        printLine('='); std::cout << " (Press Enter)"; std::cin.get();
    }

    void GameView::renderWonderDetail(const Wonder& w) {
        clearScreen(); printLine('='); printCentered("INFO: " + w.name);
        std::cout << "  Cost: " << formatCost(w.cost) << "\n";
        std::cout << "  Eff: "; for(auto& eff : w.effects) std::cout << eff->getDescription() << " ";
        std::cout << "\n"; printLine('='); std::cout << " (Press Enter)"; std::cin.get();
    }

    void GameView::renderTokenDetail(ProgressToken t) {
        clearScreen(); printLine('='); printCentered("TOKEN: " + getTokenName(t));
        std::cout << "  (See Manual)\n";
        printLine('='); std::cout << " (Press Enter)"; std::cin.get();
    }

    void GameView::renderDiscardPile(const std::vector<Card*>& pile) {
        clearScreen(); printLine('='); printCentered("DISCARD PILE (" + std::to_string(pile.size()) + ")");
        int idx = 1; for(auto c : pile) std::cout << "  [D" << idx++ << "] " << c->name << " (" << getTypeStr(c->type) << ")\n";
        printLine('='); std::cout << " (Press Enter)"; std::cin.get();
    }

    void GameView::renderFullLog(const std::vector<std::string>& log) {
        clearScreen(); printLine('='); printCentered("GAME LOG");
        for(const auto& l : log) std::cout << " " << l << "\n";
        printLine('='); std::cout << " (Press Enter)"; std::cin.get();
    }

    // ==========================================================
    //  核心交互循环：promptHumanAction
    // ==========================================================

    Action GameView::promptHumanAction(const GameModel& model, GameState state) {
        Action act;
        act.type = static_cast<ActionType>(-1);

        while (true) {
            renderGame(model, state);

            std::cout << "\n " << model.getCurrentPlayer()->name << " > ";

            std::string line;
            if (!std::getline(std::cin, line)) { act.type = ActionType::DISCARD_FOR_COINS; return act; }
            if (line.empty()) continue;

            clearLastError();
            std::stringstream ss(line);
            std::string cmd; ss >> cmd;
            std::string arg1, arg2; ss >> arg1 >> arg2;

            if (cmd == "log") { renderFullLog(model.gameLog); continue; }

            if (cmd == "detail") {
                int pIdx = -1;
                if (arg1 == "1") pIdx = 0;
                else if (arg1 == "2") pIdx = 1;

                if (pIdx != -1) {
                    renderPlayerDetailFull(*model.players[pIdx], *model.players[1-pIdx]);
                } else {
                    renderPlayerDetailFull(*model.getCurrentPlayer(), *model.getOpponent());
                }
                continue;
            }

            if (cmd == "info") {
                int cId = parseId(arg1, 'C'); int wId = parseId(arg1, 'W'); int sId = parseId(arg1, 'S'); int tId = parseId(arg1, 'T'); int dId = parseId(arg1, 'D');

                bool found = false;
                if (!found && cId!=-1 && ctx.cardIdMap.count(cId)) {
                    for(const auto& c : model.allCards) if(c.id == ctx.cardIdMap[cId]) { renderCardDetail(c); found=true; break; }
                }
                if (!found && wId!=-1 && ctx.wonderIdMap.count(wId)) {
                    for(const auto& w : model.allWonders) if(w.id == ctx.wonderIdMap[wId]) { renderWonderDetail(w); found=true; break; }
                }
                if (!found && sId!=-1) {
                    if (ctx.tokenIdMap.count(sId)) { renderTokenDetail(ctx.tokenIdMap[sId]); found=true; }
                    else if (ctx.boxTokenIdMap.count(sId)) { renderTokenDetail(ctx.boxTokenIdMap[sId]); found=true; }
                }
                if (!found && tId!=-1 && ctx.oppCardIdMap.count(tId)) {
                    for(const auto& c : model.allCards) if(c.id == ctx.oppCardIdMap[tId]) { renderCardDetail(c); found=true; break; }
                }

                if (!found) setLastError("ID not found or not visible in current context.");
                continue;
            }

            if (state == GameState::WONDER_DRAFT_PHASE_1 || state == GameState::WONDER_DRAFT_PHASE_2) {
                if (cmd == "pick") {
                    int idx = parseId(arg1, ' ');
                    if (idx >= 1 && idx <= (int)model.draftPool.size()) {
                        act.type = ActionType::DRAFT_WONDER;
                        act.targetWonderId = model.draftPool[idx-1]->id;
                        return act;
                    } else setLastError("Invalid index.");
                } else setLastError("Use 'pick <N>'.");
            }
            else if (state == GameState::WAITING_FOR_TOKEN_SELECTION_PAIR || state == GameState::WAITING_FOR_TOKEN_SELECTION_LIB) {
                bool isBox = (state == GameState::WAITING_FOR_TOKEN_SELECTION_LIB);
                if (cmd == "select") {
                    int id = parseId(arg1, 'S');
                    auto& map = isBox ? ctx.boxTokenIdMap : ctx.tokenIdMap;
                    if (map.count(id)) {
                        act.type = ActionType::SELECT_PROGRESS_TOKEN;
                        act.selectedToken = map[id];
                        return act;
                    } else setLastError("Invalid Token ID (S1, S2...).");
                } else setLastError("Use 'select <ID>'.");
            }
            else if (state == GameState::WAITING_FOR_DESTRUCTION) {
                if (cmd == "destroy") {
                    int id = parseId(arg1, 'T');
                    if (ctx.oppCardIdMap.count(id)) {
                        act.type = ActionType::SELECT_DESTRUCTION;
                        act.targetCardId = ctx.oppCardIdMap[id];
                        return act;
                    } else setLastError("Invalid Target ID (T1, T2...).");
                }
                else if (cmd == "skip") {
                    act.type = ActionType::SELECT_DESTRUCTION;
                    act.targetCardId = "";
                    return act;
                } else setLastError("Use 'destroy <ID>' or 'skip'.");
            }
            else if (state == GameState::WAITING_FOR_DISCARD_BUILD) {
                if (cmd == "resurrect") {
                    int id = parseId(arg1, 'D');
                    if (ctx.discardIdMap.count(id)) {
                        act.type = ActionType::SELECT_FROM_DISCARD;
                        act.targetCardId = ctx.discardIdMap[id];
                        return act;
                    } else setLastError("Invalid Discard ID (D1...).");
                } else setLastError("Use 'resurrect <ID>'.");
            }
            else if (state == GameState::WAITING_FOR_START_PLAYER_SELECTION) {
                if (cmd == "choose") {
                    if (arg1 == "me") {
                        act.type = ActionType::CHOOSE_STARTING_PLAYER;
                        act.targetCardId = "ME";
                        return act;
                    } else if (arg1 == "opponent" || arg1 == "opp") {
                        act.type = ActionType::CHOOSE_STARTING_PLAYER;
                        act.targetCardId = "OPPONENT";
                        return act;
                    } else setLastError("Choose 'me' or 'opponent'.");
                } else setLastError("Use 'choose me' or 'choose opponent'.");
            }
            else {
                if (cmd == "build" || cmd == "discard") {
                    int id = parseId(arg1, 'C');
                    if (ctx.cardIdMap.count(id)) {
                        act.type = (cmd == "build") ? ActionType::BUILD_CARD : ActionType::DISCARD_FOR_COINS;
                        act.targetCardId = ctx.cardIdMap[id];
                        return act;
                    } else setLastError("Invalid Card ID (C1...).");
                }
                else if (cmd == "wonder") {
                    int cId = parseId(arg1, 'C');
                    int wId = parseId(arg2, 'W');
                    if (ctx.cardIdMap.count(cId) && ctx.wonderIdMap.count(wId)) {
                        act.type = ActionType::BUILD_WONDER;
                        act.targetCardId = ctx.cardIdMap[cId];
                        act.targetWonderId = ctx.wonderIdMap[wId];
                        return act;
                    } else setLastError("Format: wonder C<ID> W<ID>");
                }
                else if (cmd == "pile") { renderDiscardPile(model.board->discardPile); }
                else setLastError("Unknown command.");
            }
        }
    }
}