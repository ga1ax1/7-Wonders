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
    //  基础工具与颜色
    // ==========================================================

    void GameView::clearScreen() {
        // ANSI escape code for clear screen and reset cursor
        std::cout << "\033[2J\033[1;1H";
    }

    void GameView::printLine(char c, int width) {
        std::cout << std::string(width, c) << "\n";
    }

    void GameView::printCentered(const std::string& text, int width) {
        int len = 0;
        bool inEsc = false;
        // 计算可见字符长度（忽略ANSI码）
        for(char c : text) {
            if(c == '\033') inEsc = true;
            if(!inEsc) len++;
            if(inEsc && c == 'm') inEsc = false;
        }

        int padding = (width - len) / 2;
        if (padding > 0) std::cout << std::string(padding, ' ');
        std::cout << text << "\n";
    }

    // ANSI Colors
    std::string GameView::getCardColorCode(CardType t) {
        switch(t) {
            case CardType::RAW_MATERIAL: return "\033[33m"; // Yellow/Brownish
            case CardType::MANUFACTURED: return "\033[90m"; // Grey
            case CardType::CIVILIAN: return "\033[34m";     // Blue
            case CardType::SCIENTIFIC: return "\033[32m";   // Green
            case CardType::COMMERCIAL: return "\033[93m";   // Bright Yellow
            case CardType::MILITARY: return "\033[31m";     // Red
            case CardType::GUILD: return "\033[35m";        // Purple
            case CardType::WONDER: return "\033[36m";       // Cyan
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

    void GameView::printError(const std::string& msg) {
        std::cout << "\033[91m[ERROR] " << msg << "\033[0m\n";
    }

    void GameView::printTurnInfo(const Player* player) {
        // Dashboard 中已包含，此处留空或简单打印
    }

    // ==========================================================
    //  文本格式化辅助
    // ==========================================================

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

    std::string GameView::getTokenShortName(ProgressToken t) {
         return getTokenName(t); // 可以优化为缩写
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
        // W:2 C:0 S:1 G:0 P:1
        ss << "W:" << p.fixedResources.at(ResourceType::WOOD) << " ";
        ss << "C:" << p.fixedResources.at(ResourceType::CLAY) << " ";
        ss << "S:" << p.fixedResources.at(ResourceType::STONE) << " ";
        ss << "G:" << p.fixedResources.at(ResourceType::GLASS) << " ";
        ss << "P:" << p.fixedResources.at(ResourceType::PAPER);
        return ss.str();
    }

    // ==========================================================
    //  渲染模块
    // ==========================================================

    void GameView::renderMainMenu() {
        clearScreen();
        std::cout << "\n";
        printLine('=', 80);
        printCentered("\033[1;33m7   W O N D E R S    D U E L\033[0m");
        printLine('=', 80);
        std::cout << "\n";
        printCentered("Please Select Game Mode:");
        std::cout << "\n";

        std::string indent(28, ' ');
        std::cout << indent << "[1] Human vs Human\n";
        std::cout << indent << "[2] Human vs AI (Recommended)\n";
        std::cout << indent << "[3] AI vs AI (Watch Mode)\n";
        std::cout << indent << "[4] Quit Game\n";

        std::cout << "\n";
        printLine('=', 80);
        std::cout << "  Input > ";
    }

    void GameView::renderMilitaryTrack(const Board& board) {
        // Position: -9 to 9. Length 19.
        int pos = board.militaryTrack.position;
        std::stringstream ss;

        ss << "        P1  ";

        // Render Track
        for (int i = -9; i <= 9; ++i) {
            if (i == pos) ss << "\033[1;31m@\033[0m "; // Conflict Pawn
            else if (i == 0) ss << "| ";
            else ss << "- ";
        }

        ss << " P2";
        std::cout << ss.str() << "\n";

        // Loot Tokens Visualization (Optional logic for Loot lines)
        // Design calls for [$ 2] [$ 5] above the track.
        // 简单实现：只在 track 上方显示状态
        std::string lootLine(80, ' ');
        // 映射 -9..9 到 字符串大概位置
        // 假设每个格占2字符 "@ "，起始偏移12字符 ( "        P1  " len=12)
        // P1 Loot: -3, -6. P2 Loot: 3, 6
        // -6 index: 12 + (-6 - (-9))*2 = 12 + 6 = 18
        // ... logic simplified:
        std::cout << "            "
                  << (board.militaryTrack.lootTokens[1] ? "[$ 5] " : "      ") // -6
                  << (board.militaryTrack.lootTokens[0] ? "[$ 2]" : "     ")   // -3
                  << "               " // space for 0 area
                  << (board.militaryTrack.lootTokens[2] ? "[$ 2] " : "      ") // 3
                  << (board.militaryTrack.lootTokens[3] ? "[$ 5]" : "     ")   // 6
                  << "\n";
    }

    void GameView::renderProgressTokens(const std::vector<ProgressToken>& tokens) {
        if (tokens.empty()) return;
        std::cout << "[TOKENS] ";
        for (size_t i = 0; i < tokens.size(); ++i) {
            std::cout << "\033[32m[S" << (i+1) << "]" << getTokenShortName(tokens[i]) << "\033[0m  ";
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

    void GameView::renderPlayerDashboard(const Player& p, bool isCurrent, const Player& opp) {
        std::string nameTag = isCurrent ? ("\033[1;36m[" + p.name + "]\033[0m") : ("[" + p.name + "]");

        // Line 1: Stats
        // Calc display VP (raw - coins) for cleaner look if desired, or just raw
        int displayVP = p.getScore(opp) - (p.coins/3);
        std::cout << nameTag
                  << " Coins:\033[33m" << p.coins << "\033[0m"
                  << " VP:\033[36m" << displayVP << "\033[0m "
                  << formatResourcesCompact(p) << "\n";

        // Line 2: Wonders
        std::cout << "Wonder: ";
        for(auto w : p.builtWonders) {
            std::cout << "\033[32m[X]" << w->name << "\033[0m  ";
        }
        for(auto w : p.unbuiltWonders) {
            std::cout << "[ ]" << w->name << "  ";
        }
        std::cout << "\n";
        printLine('-');
    }

    void GameView::renderPyramid(const GameModel& model) {
        // Info Line
        int deckSize = 0; // In a real game, this would be drawn pile size, but here all are in pyramid
        // Actually, we can just show remaining in pyramid
        int remaining = model.getRemainingCardCount();
        int discardCount = model.board->discardPile.size();

        std::cout << "           PYRAMID: " << remaining << " cards left   |   DISCARD: " << discardCount << " cards\n";

        const auto& slots = model.board->cardStructure.slots;
        if (slots.empty()) return;

        // Group by row
        std::map<int, std::vector<const CardSlot*>> rows;
        int maxRow = 0;
        for (const auto& slot : slots) {
            rows[slot.row].push_back(&slot);
            if (slot.row > maxRow) maxRow = slot.row;
        }

        // Render each row
        for (int r = 0; r <= maxRow; ++r) {
            const auto& rowSlots = rows[r];
            if (rowSlots.empty()) continue;

            // Calculate padding to center the pyramid
            // Assume each card takes approx 10 chars "[ Name ] "
            // Max width is approx 70 chars. 80 screen width.
            // Simple heuristic based on number of cards in row and Age structure
            int cardWidth = 11; // "[1234567]  "
            int rowLen = rowSlots.size() * cardWidth;
            int padding = (80 - rowLen) / 2;

            // Age 3 specific tweak for "Snake" layout offset if needed,
            // but standard centering works surprisingly well for all shapes.

            std::cout << std::string(std::max(0, padding), ' ');

            for (const auto* slot : rowSlots) {
                if (slot->isRemoved) {
                    std::cout << "           "; // Empty space
                } else if (!slot->isFaceUp) {
                    std::cout << " [\033[90m ? ? ? \033[0m] ";
                } else {
                    Card* c = slot->cardPtr;
                    std::string color = getCardColorCode(c->type);
                    std::string name = c->name.substr(0, 7); // Truncate to fit
                    std::cout << " [" << color << std::left << std::setw(7) << name << getResetColor() << "] ";
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

    void GameView::renderCommandHelp() {
        std::cout << " [COMMANDS]\n";
        std::cout << " - build <Name>     : Build a card (e.g., 'build Lumber')\n";
        std::cout << " - wonder <C> <W>   : Build Wonder <W> using card <C>\n";
        std::cout << " - discard <Name>   : Discard card <Name> for coins\n";
        std::cout << " - info <Name>      : Check info (Card/Wonder/Token)\n";
        std::cout << " - pile / log       : View Discard Pile / Full Log\n";
        std::cout << " - detail <1/2>     : View player detail\n";
    }

    void GameView::renderGame(const GameModel& model) {
        clearScreen();
        renderHeader(model);

        // P1 (Top)
        renderPlayerDashboard(*model.players[0], model.currentPlayerIndex == 0, *model.players[1]);

        // Pyramid
        renderPyramid(model);

        // P2 (Bottom)
        renderPlayerDashboard(*model.players[1], model.currentPlayerIndex == 1, *model.players[0]);

        // Footer
        renderActionLog(model.gameLog);
        renderCommandHelp();
    }

    // ==========================================================
    //  辅助详情页
    // ==========================================================

    void GameView::renderPlayerDetailFull(const Player& p, const Player& opp) {
        clearScreen();
        printLine('=');
        printCentered("DETAIL: " + p.name);
        std::cout << "\n";

        std::cout << " [1] BASIC INFO\n";
        std::cout << "     Coins: " << p.coins
                  << "   VP: " << p.getScore(opp) << "\n\n";

        std::cout << " [2] RESOURCES & TRADE\n";
        std::cout << "     Type      Amount    Buy Cost (Pay to Bank)\n";
        std::cout << "     ------------------------------------------\n";
        std::vector<ResourceType> types = {ResourceType::WOOD, ResourceType::CLAY, ResourceType::STONE, ResourceType::GLASS, ResourceType::PAPER};
        for (auto t : types) {
            int amt = 0;
            if (p.fixedResources.count(t)) amt = p.fixedResources.at(t);
            int cost = p.getTradingPrice(t, opp);
            std::cout << "     " << std::left << std::setw(10) << resourceName(t)
                      << std::setw(10) << amt
                      << cost << "$\n";
        }
        std::cout << "\n";

        std::cout << " [3] SCIENCE\n";
        std::cout << "     Symbols: ";
        for(auto [s, c] : p.scienceSymbols) {
            if(c > 0 && s != ScienceSymbol::NONE) std::cout << "[" << (int)s << "]x" << c << " "; // 简化显示
        }
        std::cout << "\n\n";

        std::cout << " [4] BUILT CARDS (" << p.builtCards.size() << ")\n";
        std::map<CardType, int> counts;
        for(auto c : p.builtCards) counts[c->type]++;
        std::cout << "     Blue:" << counts[CardType::CIVILIAN]
                  << " Green:" << counts[CardType::SCIENTIFIC]
                  << " Red:" << counts[CardType::MILITARY]
                  << " Yellow:" << counts[CardType::COMMERCIAL] << "\n";

        std::cout << "\n======================================================================\n";
        std::cout << " (Press Enter to return)";
        std::cin.get();
    }

    void GameView::renderCardDetail(const Card& c) {
        clearScreen();
        printLine('=');
        printCentered("INFO: " + c.name);
        std::cout << "\n";

        std::cout << "  Type:   " << getTypeStr(c.type) << "\n";
        std::cout << "  Cost:   " << formatCost(c.cost) << "\n\n";

        std::cout << "  --- EFFECTS ---\n";
        for(auto& eff : c.effects) {
            std::cout << "  * " << eff->getDescription() << "\n";
        }
        std::cout << "\n";

        std::cout << "  --- CHAIN ---\n";
        if(!c.chainTag.empty()) std::cout << "  Chain Symbol: [" << c.chainTag << "]\n";
        if(!c.requiresChainTag.empty()) std::cout << "  Builds for free with: [" << c.requiresChainTag << "]\n";

        printLine('=');
        std::cout << " (Press Enter to return)";
        std::cin.get();
    }

    void GameView::renderWonderDetail(const Wonder& w) {
        clearScreen();
        printLine('=');
        printCentered("INFO: " + w.name);
        std::cout << "\n";
        std::cout << "  Type:   [Wonder]\n";
        std::cout << "  Cost:   " << formatCost(w.cost) << "\n\n";
        std::cout << "  --- EFFECTS ---\n";
        for(auto& eff : w.effects) {
            std::cout << "  * " << eff->getDescription() << "\n";
        }
        printLine('=');
        std::cout << " (Press Enter to return)";
        std::cin.get();
    }

    void GameView::renderTokenDetail(ProgressToken t) {
        clearScreen();
        printLine('=');
        printCentered("TOKEN: " + getTokenName(t));
        std::cout << "\n";
        // 简单描述，实际可扩展
        std::cout << "  Effect: See manual for details.\n";
        printLine('=');
        std::cout << " (Press Enter to return)";
        std::cin.get();
    }

    void GameView::renderDiscardPile(const std::vector<Card*>& pile) {
        clearScreen();
        printLine('=');
        printCentered("DISCARD PILE");
        std::cout << " Total Cards: " << pile.size() << "\n\n";

        int idx = 1;
        for(auto c : pile) {
            std::cout << "  " << idx++ << ". " << c->name << " (" << getTypeStr(c->type) << ")\n";
        }

        printLine('=');
        std::cout << " (Press Enter to return)";
        std::cin.get();
    }

    void GameView::renderFullLog(const std::vector<std::string>& log) {
        clearScreen();
        printLine('=');
        printCentered("GAME LOG");
        std::cout << "\n";
        for(const auto& l : log) {
            std::cout << " " << l << "\n";
        }
        printLine('=');
        std::cout << " (Press Enter to return)";
        std::cin.get();
    }

    // ==========================================================
    //  交互核心
    // ==========================================================

    // 辅助：模糊匹配
    int findMatch(const std::vector<std::string>& names, const std::string& query) {
        std::string q = query;
        std::transform(q.begin(), q.end(), q.begin(), ::tolower);
        // Exact
        for(size_t i=0; i<names.size(); ++i) {
            std::string n = names[i];
            std::transform(n.begin(), n.end(), n.begin(), ::tolower);
            if (n == q) return i;
        }
        // Partial
        for(size_t i=0; i<names.size(); ++i) {
            std::string n = names[i];
            std::transform(n.begin(), n.end(), n.begin(), ::tolower);
            if (n.find(q) != std::string::npos) return i;
        }
        return -1;
    }

    Action GameView::promptHumanAction(const GameModel& model, GameState state) {
        Action act;
        act.type = static_cast<ActionType>(-1); // Invalid init

        // Loop until a valid game action is formed (handling aux commands internally)
        while (true) {

            // 1. 特殊状态处理 (Draft, Discard Select, etc)
            if (model.currentAge == 0 && state == GameState::WONDER_DRAFT_PHASE_1) {
                renderGame(model); // 这里应该有一个专门的 Draft UI，复用 renderGame 也可以，或者单独写
                std::cout << "\n--- WONDER DRAFT ---\n";
                int idx = 1;
                for(auto w : model.draftPool) {
                    std::cout << "[" << idx++ << "] " << w->name << " (Cost: " << formatCost(w->cost) << ")\n";
                }
                std::cout << "Commands: pick <1-4>, detail, info <name>\n> ";
            } else {
                // 正常 Dashboard
                renderGame(model);
                std::cout << "\n " << model.getCurrentPlayer()->name << " > ";
            }

            std::string line;
            if (!std::getline(std::cin, line)) {
                // EOF safety
                act.type = ActionType::DISCARD_FOR_COINS; // Fail safe
                return act;
            }
            if (line.empty()) continue;

            std::stringstream ss(line);
            std::string cmd;
            ss >> cmd;
            std::string arg1, arg2;

            // Read rest of line as args
            std::string remainder;
            std::getline(ss, remainder);
            size_t first = remainder.find_first_not_of(' ');
            if (std::string::npos != first) remainder = remainder.substr(first);

            // --- 指令解析 ---

            // Draft Phase Specific
            if (model.currentAge == 0) {
                if (cmd == "pick") {
                    try {
                        int idx = std::stoi(remainder);
                        if (idx >= 1 && idx <= model.draftPool.size()) {
                            act.type = ActionType::DRAFT_WONDER;
                            act.targetWonderId = model.draftPool[idx-1]->id;
                            return act;
                        }
                    } catch (...) {}
                }
                // Fallthrough to standard commands like detail/info
            }

            // Standard Commands
            if (cmd == "build" || cmd == "discard") {
                // Find card in pyramid
                auto avail = model.board->cardStructure.getAvailableCards();
                std::vector<std::string> names;
                std::vector<Card*> cards;
                for(auto s : avail) { if(s->cardPtr) { names.push_back(s->cardPtr->name); cards.push_back(s->cardPtr); } }

                int idx = findMatch(names, remainder);
                if (idx != -1) {
                    act.type = (cmd == "build") ? ActionType::BUILD_CARD : ActionType::DISCARD_FOR_COINS;
                    act.targetCardId = cards[idx]->id;
                    return act;
                } else {
                    printError("Card not found in available slots.");
                    std::cout << "(Press Enter)"; std::cin.get();
                }
            }
            else if (cmd == "wonder") {
                // Format: wonder <CardName> <WonderName>
                // We need to parse two args manually or interactively
                // Let's try simple split
                std::stringstream argSS(remainder);
                std::string cName, wName;
                argSS >> cName;
                std::getline(argSS, wName);
                // trim wName
                first = wName.find_first_not_of(' ');
                if (std::string::npos != first) wName = wName.substr(first);

                // Find Card
                auto avail = model.board->cardStructure.getAvailableCards();
                std::vector<std::string> cNames;
                std::vector<Card*> cards;
                for(auto s : avail) { if(s->cardPtr) { cNames.push_back(s->cardPtr->name); cards.push_back(s->cardPtr); } }
                int cIdx = findMatch(cNames, cName);

                // Find Wonder
                auto& unbuilt = model.getCurrentPlayer()->unbuiltWonders;
                std::vector<std::string> wNames;
                for(auto w : unbuilt) wNames.push_back(w->name);
                int wIdx = findMatch(wNames, wName);

                if (cIdx != -1 && wIdx != -1) {
                    act.type = ActionType::BUILD_WONDER;
                    act.targetCardId = cards[cIdx]->id;
                    act.targetWonderId = unbuilt[wIdx]->id;
                    return act;
                } else {
                    printError("Invalid Card or Wonder name.");
                    std::cout << "(Press Enter)"; std::cin.get();
                }
            }
            else if (cmd == "detail") {
                if (remainder == "2") renderPlayerDetailFull(*model.players[1], *model.players[0]);
                else renderPlayerDetailFull(*model.players[0], *model.players[1]);
            }
            else if (cmd == "pile") {
                renderDiscardPile(model.board->discardPile);
            }
            else if (cmd == "log") {
                renderFullLog(model.gameLog);
            }
            else if (cmd == "info") {
                // Try searching Cards (All), Wonders, Tokens
                bool found = false;
                for(const auto& c : model.allCards) {
                    if(findMatch({c.name}, remainder) != -1) { renderCardDetail(c); found=true; break; }
                }
                if(!found) {
                    for(const auto& w : model.allWonders) {
                        if(findMatch({w.name}, remainder) != -1) { renderWonderDetail(w); found=true; break; }
                    }
                }
                if (!found) {
                     // Tokens
                     std::vector<ProgressToken> tokens = {ProgressToken::AGRICULTURE, ProgressToken::LAW, /*...*/};
                     // simplify: logic in helper
                     printError("Not found."); std::cout << "(Press Enter)"; std::cin.get();
                }
            }

            // Special State: Discard Selection
            if (state == GameState::WAITING_FOR_DISCARD_BUILD && cmd == "resurrect") {
                 // Logic for mausoleum...
            }
        }
    }

}