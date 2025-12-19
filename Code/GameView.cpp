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

    void GameView::clearScreen() {
        std::cout << "\033[2J\033[1;1H";
    }

    void GameView::printLine(char c, int width) {
        std::cout << std::string(width, c) << "\n";
    }

    void GameView::printCentered(const std::string& text, int width) {
        int len = 0;
        bool inEsc = false;
        for(char c : text) {
            if(c == '\033') inEsc = true;
            if(!inEsc) len++;
            if(inEsc && c == 'm') inEsc = false;
        }
        int padding = (width - len) / 2;
        if (padding > 0) std::cout << std::string(padding, ' ');
        std::cout << text << "\n";
    }

    int GameView::parseId(const std::string& input, char prefix) {
        if (input.empty()) return -1;
        std::string numPart = input;

        // 允许 "C1" 或 "1"
        if (toupper(input[0]) == prefix) {
            if (input.size() > 1) numPart = input.substr(1);
            else return -1;
        }

        try {
            return std::stoi(numPart);
        } catch (...) {
            return -1;
        }
    }

    void GameView::setLastError(const std::string& msg) {
        m_lastError = msg;
    }

    void GameView::clearLastError() {
        m_lastError = "";
    }

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

    void GameView::printMessage(const std::string& msg) {
        std::cout << "\033[96m[INFO] " << msg << "\033[0m\n";
    }

    // 已弃用，直接在 Dashboard 中显示
    // void GameView::printTurnInfo(const Player* player) {}

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
            case ResourceType::WOOD: return "Wood";
            case ResourceType::STONE: return "Stone";
            case ResourceType::CLAY: return "Clay";
            case ResourceType::PAPER: return "Paper";
            case ResourceType::GLASS: return "Glass";
        }
        return "?";
    }

    std::string GameView::formatCost(const ResourceCost& cost) {
        std::stringstream ss;
        if(cost.coins > 0) ss << "$" << cost.coins << " ";
        for(auto [r,v]: cost.resources) {
            if(v>0) ss << v << resourceName(r).substr(0,1) << " ";
        }
        std::string s = ss.str();
        if(s.empty()) return "Free";
        return s;
    }

    std::string GameView::formatResourcesCompact(const Player& p) {
        std::stringstream ss;
        ss << "W:" << p.fixedResources.at(ResourceType::WOOD) << " ";
        ss << "C:" << p.fixedResources.at(ResourceType::CLAY) << " ";
        ss << "S:" << p.fixedResources.at(ResourceType::STONE) << " ";
        ss << "G:" << p.fixedResources.at(ResourceType::GLASS) << " ";
        ss << "P:" << p.fixedResources.at(ResourceType::PAPER);
        return ss.str();
    }

    // ==========================================================
    //  主渲染逻辑
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

    void GameView::renderErrorMessage() {
        if (!m_lastError.empty()) {
            std::cout << "\033[1;31m [!] " << m_lastError << "\033[0m\n";
        }
    }

    // --- 奇迹轮抽阶段 ---
    void GameView::renderDraftPhase(const GameModel& model) {
        clearScreen();
        printLine('=');
        printCentered("\033[1;36mWONDER DRAFT PHASE\033[0m");
        printLine('=');

        const Player* p = model.getCurrentPlayer();
        std::string nameTag = "\033[1;33m[" + p->name + "]\033[0m";
        std::cout << "  " << nameTag << " Select a Wonder to keep:\n";

        ctx.draftWonderIds.clear();
        int idx = 1;
        for (const auto& w : model.draftPool) {
            ctx.draftWonderIds.push_back(w->id);
            std::cout << "  [" << idx << "] \033[1;37m" << std::left << std::setw(20) << w->name << "\033[0m";
            std::cout << " Cost: " << formatCost(w->cost) << " ";
            std::cout << " Eff: ";
            for(auto& eff : w->effects) std::cout << eff->getDescription() << " ";
            std::cout << "\n";
            idx++;
        }
        printLine('-');
        renderErrorMessage();
        renderCommandHelp(true);
    }

    // --- 主游戏阶段组件 ---

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

        // Loot Tokens - 紧凑显示
        std::cout << "            "
                  << (board.militaryTrack.lootTokens[1] ? "[$ 5] " : "      ")
                  << (board.militaryTrack.lootTokens[0] ? "[$ 2]" : "     ")
                  << "               "
                  << (board.militaryTrack.lootTokens[2] ? "[$ 2] " : "      ")
                  << (board.militaryTrack.lootTokens[3] ? "[$ 5]" : "     ")
                  << "\n";
    }

    void GameView::renderProgressTokens(const std::vector<ProgressToken>& tokens) {
        ctx.tokenIdMap.clear();
        if (tokens.empty()) return;
        std::cout << "[TOKENS] ";
        for (size_t i = 0; i < tokens.size(); ++i) {
            int displayId = i + 1;
            ctx.tokenIdMap[displayId] = tokens[i];
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
        renderProgressTokens(model.board->availableProgressTokens);
        printLine('-');
    }

    void GameView::renderPlayerDashboard(const Player& p, bool isCurrent, const Player& opp, int& wonderCounter) {
        std::string nameTag = isCurrent ? ("\033[1;36m[" + p.name + "]\033[0m") : ("[" + p.name + "]");

        int displayVP = p.getScore(opp) - (p.coins/3);
        std::cout << nameTag
                  << " Coin:\033[33m" << p.coins << "\033[0m"
                  << " VP:\033[36m" << displayVP << "\033[0m "
                  << formatResourcesCompact(p) << "\n";

        std::cout << "Wonder: ";
        // 显示已建成奇迹
        for(auto w : p.builtWonders) {
            // 注册 ID 供查询
            int currentId = wonderCounter++;
            ctx.wonderIdMap[currentId] = w->id;
            std::cout << "\033[32m[W" << currentId << "][X]" << w->name << "\033[0m  ";
        }

        // 显示未建成奇迹
        for(auto w : p.unbuiltWonders) {
            int currentId = wonderCounter++;
            ctx.wonderIdMap[currentId] = w->id;
            std::cout << "[W" << currentId << "][ ]" << w->name << "  ";
        }
        std::cout << "\n";
        printLine('-');
    }

    void GameView::renderPyramid(const GameModel& model) {
        int remaining = model.getRemainingCardCount();
        int discardCount = model.board->discardPile.size();

        std::cout << "           PYRAMID: " << remaining << " left | DISCARD: " << discardCount << "\n";

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
            int padding = (80 - rowLen) / 2;
            std::cout << std::string(std::max(0, padding), ' ');

            for (const auto* slot : rowSlots) {
                int absIndex = (&(*slot) - &slots[0]) + 1;

                if (slot->isRemoved) {
                    std::cout << "           ";
                } else if (!slot->isFaceUp) {
                    std::cout << " [\033[90m ? ? ? \033[0m] ";
                } else {
                    Card* c = slot->cardPtr;
                    ctx.cardIdMap[absIndex] = c->id;

                    std::string color = getCardColorCode(c->type);
                    std::stringstream ss;
                    ss << " C" << absIndex << " ";
                    std::string label = ss.str();
                    while(label.length() < 7) label += " ";

                    std::cout << " [" << color << label << getResetColor() << "] ";
                }
            }
            std::cout << "\n";
        }
        printLine('-');
    }

    void GameView::renderActionLog(const std::vector<std::string>& log) {
        std::cout << " [LAST ACTION]\n";
        if (log.empty()) {
            std::cout << " > Game Started.\n";
        } else {
            size_t count = std::min((size_t)2, log.size());
            for (size_t i = 0; i < count; ++i) {
                std::cout << " > " << log[log.size() - count + i] << "\n";
            }
        }
        printLine('=');
    }

    void GameView::renderCommandHelp(bool isDraft) {
        std::cout << " [CMD] ";
        if (isDraft) {
            std::cout << "pick <ID>, detail, info <ID>\n";
        } else {
            std::cout << "build/discard <CID>, wonder <CID> <WID>, info <ID>, pile, log, detail\n";
        }
    }

    void GameView::renderGame(const GameModel& model) {
        clearScreen();
        // 重置 Context
        ctx.wonderIdMap.clear();
        int wonderCounter = 1; // 全局计数器，确保 P1 W1-W4, P2 W5-W8 (或其他数量)

        renderHeader(model);
        renderPlayerDashboard(*model.players[0], model.currentPlayerIndex == 0, *model.players[1], wonderCounter);
        renderPyramid(model);
        renderPlayerDashboard(*model.players[1], model.currentPlayerIndex == 1, *model.players[0], wonderCounter);
        renderActionLog(model.gameLog);
        renderErrorMessage();
        renderCommandHelp(false);
    }

    // External interface for AI loop
    void GameView::renderGameForAI(const GameModel& model) {
        renderGame(model);
    }

    // ==========================================================
    //  详情页
    // ==========================================================

    void GameView::renderPlayerDetailFull(const Player& p, const Player& opp) {
        clearScreen();
        printLine('=');
        printCentered("DETAIL: " + p.name);
        std::cout << " [1] BASIC: Coins " << p.coins << " | VP " << p.getScore(opp) << "\n";
        std::cout << " [2] RESOURCES: " << formatResourcesCompact(p) << "\n";
        std::cout << "     Buy Costs (W/C/S/G/P): ";
        std::vector<ResourceType> types = {ResourceType::WOOD, ResourceType::CLAY, ResourceType::STONE, ResourceType::GLASS, ResourceType::PAPER};
        for (auto t : types) std::cout << p.getTradingPrice(t, opp) << "$ ";
        std::cout << "\n";

        std::cout << " [3] SCIENCE: ";
        for(auto [s, c] : p.scienceSymbols) {
            if(c > 0 && s != ScienceSymbol::NONE) std::cout << "[" << (int)s << "]x" << c << " ";
        }
        std::cout << "\n";

        std::cout << " [4] WONDERS:\n";
        for(auto w : p.builtWonders) std::cout << "     [Built] " << w->name << "\n";
        for(auto w : p.unbuiltWonders) std::cout << "     [Plan ] " << w->name << "\n";

        printLine('=');
        std::cout << " (Press Enter)";
        std::cin.get();
    }

    void GameView::renderCardDetail(const Card& c) {
        clearScreen();
        printLine('=');
        printCentered("INFO: " + c.name);
        std::cout << "  Type: " << getTypeStr(c.type) << " | Cost: " << formatCost(c.cost) << "\n";
        std::cout << "  Eff: ";
        for(auto& eff : c.effects) std::cout << eff->getDescription() << " ";
        std::cout << "\n";
        printLine('=');
        std::cout << " (Press Enter)";
        std::cin.get();
    }

    void GameView::renderWonderDetail(const Wonder& w) {
        clearScreen();
        printLine('=');
        printCentered("INFO: " + w.name);
        std::cout << "  Cost: " << formatCost(w.cost) << "\n";
        std::cout << "  Eff: ";
        for(auto& eff : w.effects) std::cout << eff->getDescription() << " ";
        std::cout << "\n";
        printLine('=');
        std::cout << " (Press Enter)";
        std::cin.get();
    }

    void GameView::renderTokenDetail(ProgressToken t) {
        clearScreen();
        printLine('=');
        printCentered("TOKEN: " + getTokenName(t));
        std::cout << "  (Effect description placeholder)\n";
        printLine('=');
        std::cout << " (Press Enter)";
        std::cin.get();
    }

    void GameView::renderDiscardPile(const std::vector<Card*>& pile) {
        clearScreen();
        printLine('=');
        printCentered("DISCARD PILE (" + std::to_string(pile.size()) + ")");
        int idx = 1;
        for(auto c : pile) {
            std::cout << "  [D" << idx++ << "] " << c->name << " (" << getTypeStr(c->type) << ")\n";
        }
        printLine('=');
        std::cout << " (Press Enter)";
        std::cin.get();
    }

    void GameView::renderFullLog(const std::vector<std::string>& log) {
        clearScreen();
        printLine('=');
        printCentered("GAME LOG");
        for(const auto& l : log) std::cout << " " << l << "\n";
        printLine('=');
        std::cout << " (Press Enter)";
        std::cin.get();
    }

    // ==========================================================
    //  交互与输入解析
    // ==========================================================

    Action GameView::promptHumanAction(const GameModel& model, GameState state) {
        Action act;
        act.type = static_cast<ActionType>(-1);

        while (true) {
            bool isDraft = (model.currentAge == 0);

            if (isDraft) renderDraftPhase(model);
            else renderGame(model);

            std::cout << "\n " << model.getCurrentPlayer()->name << " > ";

            std::string line;
            if (!std::getline(std::cin, line)) {
                act.type = ActionType::DISCARD_FOR_COINS;
                return act;
            }

            if (line.empty()) continue;

            clearLastError();

            std::stringstream ss(line);
            std::string cmd; ss >> cmd;
            std::string arg1, arg2; ss >> arg1 >> arg2;

            // --- 1. Draft Logic ---
            if (isDraft) {
                if (cmd == "pick") {
                    int idx = parseId(arg1, ' ');
                    if (idx >= 1 && idx <= (int)ctx.draftWonderIds.size()) {
                        act.type = ActionType::DRAFT_WONDER;
                        act.targetWonderId = ctx.draftWonderIds[idx-1];
                        return act;
                    } else {
                        setLastError("Invalid Wonder ID.");
                    }
                }
                else if (cmd == "detail") { renderPlayerDetailFull(*model.getCurrentPlayer(), *model.getOpponent()); }
                else if (cmd == "info") {
                    int idx = parseId(arg1, ' ');
                     if (idx >= 1 && idx <= (int)model.draftPool.size()) {
                         renderWonderDetail(*model.draftPool[idx-1]);
                     } else {
                         setLastError("Info ID not found.");
                     }
                } else {
                     setLastError("Unknown command.");
                }

                if (act.type != static_cast<ActionType>(-1)) return act;
                continue;
            }

            // --- 2. Main Game Logic ---
            if (cmd == "build" || cmd == "discard") {
                int id = parseId(arg1, 'C');
                if (ctx.cardIdMap.count(id)) {
                    act.type = (cmd == "build") ? ActionType::BUILD_CARD : ActionType::DISCARD_FOR_COINS;
                    act.targetCardId = ctx.cardIdMap[id];
                    return act;
                } else {
                    setLastError("Card ID not found. Use C1, C2...");
                }
            }
            else if (cmd == "wonder") {
                // wonder C1 W1
                int cId = parseId(arg1, 'C');
                int wId = parseId(arg2, 'W');

                if (ctx.cardIdMap.count(cId) && ctx.wonderIdMap.count(wId)) {
                    act.type = ActionType::BUILD_WONDER;
                    act.targetCardId = ctx.cardIdMap[cId];
                    act.targetWonderId = ctx.wonderIdMap[wId];
                    return act;
                } else {
                    setLastError("Invalid format. Try 'wonder C1 W1'");
                }
            }
            else if (cmd == "info") {
                int cId = parseId(arg1, 'C');
                int wId = parseId(arg1, 'W');
                int sId = parseId(arg1, 'S');

                if (cId != -1 && ctx.cardIdMap.count(cId)) {
                    for(const auto& c : model.allCards) {
                        if (c.id == ctx.cardIdMap[cId]) { renderCardDetail(c); break; }
                    }
                }
                else if (wId != -1 && ctx.wonderIdMap.count(wId)) {
                    for(const auto& w : model.allWonders) {
                         if (w.id == ctx.wonderIdMap[wId]) { renderWonderDetail(w); break; }
                    }
                }
                else if (sId != -1 && ctx.tokenIdMap.count(sId)) {
                    renderTokenDetail(ctx.tokenIdMap[sId]);
                }
                else {
                    setLastError("ID not found. Use C1, W1, S1...");
                }
            }
            else if (cmd == "pile") renderDiscardPile(model.board->discardPile);
            else if (cmd == "log") renderFullLog(model.gameLog);
            else if (cmd == "detail") {
                 if (arg1 == "2") renderPlayerDetailFull(*model.players[1], *model.players[0]);
                 else renderPlayerDetailFull(*model.players[0], *model.players[1]);
            } else {
                setLastError("Unknown command.");
            }
        }
    }
}