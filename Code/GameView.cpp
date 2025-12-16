#include "GameView.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <sstream>
#include <limits>
#include <algorithm>
#include <map>

namespace SevenWondersDuel {

    // --- 基础工具 ---

    void GameView::clearScreen() {
        // 暴力清屏：打印大量换行符，推走旧内容
        // 这是解决 IDE (CLion/Xcode) 控制台不支持 ANSI 清屏的最通用土办法
        std::cout << std::string(100, '\n');

        // 尝试 ANSI 复位光标 (如果在支持的终端里体验更好)
        std::cout << "\033[2J\033[1;1H";
    }

    void GameView::printLine(char c, int width) {
        std::cout << std::string(width, c) << std::endl;
    }

    void GameView::renderHeader(const std::string& text) {
        printLine('=');
        std::cout << "  " << text << std::endl;
        printLine('=');
    }

    // --- 文本转换辅助 ---

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

    std::string getTokenDesc(ProgressToken t) {
        switch(t) {
            case ProgressToken::AGRICULTURE: return "6 Coins immediately, 4 VP.";
            case ProgressToken::URBANISM: return "6 Coins immediately, 4 Coins per chain.";
            case ProgressToken::STRATEGY: return "Extra shield on military buildings.";
            case ProgressToken::THEOLOGY: return "Wonders grant Play Again.";
            case ProgressToken::ECONOMY: return "Opponent pays you for trade.";
            case ProgressToken::MASONRY: return "Blue cards cost -2 resources.";
            case ProgressToken::ARCHITECTURE: return "Wonders cost -2 resources.";
            case ProgressToken::LAW: return "Science Symbol (Law).";
            case ProgressToken::MATHEMATICS: return "3 VP per Progress Token.";
            case ProgressToken::PHILOSOPHY: return "7 VP.";
            default: return "";
        }
    }

    std::string getTypeStr(CardType t) {
        switch(t) {
            case CardType::RAW_MATERIAL: return "\033[33mBrown\033[0m";
            case CardType::MANUFACTURED: return "\033[37mGrey\033[0m";
            case CardType::CIVILIAN: return "\033[34mBlue\033[0m";
            case CardType::SCIENTIFIC: return "\033[32mGreen\033[0m";
            case CardType::COMMERCIAL: return "\033[33mYellow\033[0m";
            case CardType::MILITARY: return "\033[31mRed\033[0m";
            case CardType::GUILD: return "\033[35mPurple\033[0m";
            default: return "Unknown";
        }
    }

    std::string resourceName(ResourceType r) {
        switch(r) {
            case ResourceType::WOOD: return "Wood";
            case ResourceType::STONE: return "Stone";
            case ResourceType::CLAY: return "Clay";
            case ResourceType::PAPER: return "Paper";
            case ResourceType::GLASS: return "Glass";
        }
        return "";
    }

    std::string getScienceSymbolName(ScienceSymbol s) {
        switch(s) {
            case ScienceSymbol::GLOBE: return "Globe";
            case ScienceSymbol::TABLET: return "Tablet";
            case ScienceSymbol::MORTAR: return "Mortar";
            case ScienceSymbol::COMPASS: return "Compass";
            case ScienceSymbol::WHEEL: return "Wheel";
            case ScienceSymbol::QUILL: return "Quill";
            case ScienceSymbol::LAW: return "Law";
            default: return "";
        }
    }

    std::string formatResFull(const Player& p) {
        std::stringstream ss;
        std::vector<ResourceType> types = {ResourceType::WOOD, ResourceType::STONE, ResourceType::CLAY, ResourceType::PAPER, ResourceType::GLASS};
        int count = 0;
        for(size_t i=0; i<types.size(); ++i) {
            int amt = 0;
            if (p.fixedResources.count(types[i])) amt = p.fixedResources.at(types[i]);
            ss << resourceName(types[i]) << ":" << amt << "  ";
            count++;
            if (count == 3) ss << "\n          ";
        }
        return ss.str();
    }

    std::string formatCost(const ResourceCost& cost) {
        std::stringstream ss;
        if(cost.coins > 0) ss << cost.coins << "$ ";
        for(auto [r,v]: cost.resources) {
            if(v>0) ss << v << resourceName(r) << " ";
        }
        if(ss.str().empty()) return "Free";
        return ss.str();
    }

    // --- 详情渲染 ---

    void GameView::renderCardDetail(const Card& c) {
        printLine('-');
        std::cout << " [CARD] " << c.name << " (" << getTypeStr(c.type) << ")\n";
        printLine('-');
        std::cout << " Age: " << c.age << "\n";
        std::cout << " Cost: " << formatCost(c.cost) << "\n";
        std::cout << " Effects: ";
        for(auto& eff : c.effects) std::cout << eff->getDescription() << " ";
        std::cout << "\n";
        if(!c.chainTag.empty()) std::cout << " Chain Provided: " << c.chainTag << "\n";
        if(!c.requiresChainTag.empty()) std::cout << " Chain Required: " << c.requiresChainTag << "\n";
        printLine('-');
    }

    void GameView::renderWonderDetail(const Wonder& w) {
        printLine('-');
        std::cout << " [WONDER] " << w.name << "\n";
        printLine('-');
        std::cout << " Cost: " << formatCost(w.cost) << "\n";
        std::cout << " Effects: ";
        for(auto& eff : w.effects) std::cout << eff->getDescription() << " ";
        std::cout << "\n";
        printLine('-');
    }

    void GameView::renderTokenDetail(ProgressToken t) {
        printLine('-');
        std::cout << " [TOKEN] " << getTokenName(t) << "\n";
        printLine('-');
        std::cout << " Effect: " << getTokenDesc(t) << "\n";
        printLine('-');
    }

    // --- 核心：详细玩家信息页面 ---

    void GameView::renderPlayerDetailFull(const Player& p, const Player& opp) {
        clearScreen();
        renderHeader("PLAYER DETAIL: " + p.name);

        // 1. 基础状态
        std::cout << "Coins: " << p.coins << "\n";
        std::cout << "Resources: " << formatResFull(p) << "\n\n";

        // 2. 正面效果 (Buffs)
        std::cout << "=== ACTIVE BUFFS ===\n";
        // 发展标记
        if (p.progressTokens.empty() && p.scienceSymbols.empty() && p.tradingDiscounts.empty()) {
            std::cout << "  (None)\n";
        } else {
            for(auto t : p.progressTokens) {
                std::cout << "  [Token] " << getTokenName(t) << ": " << getTokenDesc(t) << "\n";
            }
            for(auto [s, c] : p.scienceSymbols) {
                if(c > 0 && s != ScienceSymbol::NONE)
                    std::cout << "  [Science] " << getScienceSymbolName(s) << " x" << c << "\n";
            }
            for(auto [r, has] : p.tradingDiscounts) {
                if(has) std::cout << "  [Trade] Fixed cost 1 for " << resourceName(r) << "\n";
            }
        }
        std::cout << "\n";

        // 3. 建筑列表 (按颜色分类)
        std::cout << "=== BUILT CARDS ===\n";
        if (p.builtCards.empty()) {
            std::cout << "  (No buildings)\n";
        } else {
            // 简单排序：按类型
            std::vector<Card*> sorted = p.builtCards;
            std::sort(sorted.begin(), sorted.end(), [](Card* a, Card* b){
                return a->type < b->type;
            });

            CardType lastType = static_cast<CardType>(-1);
            for(auto c : sorted) {
                if (c->type != lastType) {
                    std::cout << "  " << getTypeStr(c->type) << ":\n";
                    lastType = c->type;
                }
                std::cout << "    - " << std::left << std::setw(20) << c->name << " ";
                for(auto& e : c->effects) std::cout << "[" << e->getDescription() << "] ";
                std::cout << "\n";
            }
        }
        std::cout << "\n";

        // 4. 奇迹
        std::cout << "=== WONDERS ===\n";
        for(auto w : p.builtWonders) std::cout << "  [Built] " << w->name << "\n";
        for(auto w : p.unbuiltWonders) std::cout << "  [Wait]  " << w->name << "\n";
        std::cout << "\n";

        // 5. 计分板 (近似计算)
        // 注意：getScore 包含所有分。这里我们手动列出构成。
        int total = p.getScore(opp);
        int coinPoints = p.coins / 3;
        // 简单显示
        std::cout << "=== SCORE BREAKDOWN ===\n";
        std::cout << "  Coins (" << p.coins << "/3): " << coinPoints << " VP\n";
        std::cout << "  Buildings/Wonders/Tokens: " << (total - coinPoints) << " VP\n";
        std::cout << "  -------------------------\n";
        std::cout << "  TOTAL: " << total << " VP\n";

        std::cout << "\n(Press Enter to return)";
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cin.get();
    }

    // --- 主游戏界面 ---

    void renderPlayerBox(const Player& p, bool isCurrent, const Player& opp) {
        std::string border = "-------------------------------------------------------------------------";
        std::string title = "【" + p.name + (isCurrent ? " (Turn)" : "") + "】";

        // 计算“面板分数”：为了满足用户“初始0分”的需求，我们显示不含金币的分数，
        // 或者直接显示 0 (如果是初始状态)。
        // 逻辑：总分 - 金币分。
        int rawScore = p.getScore(opp);
        int coinScore = p.coins / 3;
        int displayScore = rawScore - coinScore;

        std::cout << "┌" << border << "┐\n";
        std::cout << "  " << std::left << std::setw(50) << title
                  << "VP (Excl. Coins): " << displayScore << "\n";

        std::cout << "  Coins: " << std::left << std::setw(5) << p.coins << "  ";
        std::cout << formatResFull(p) << "\n";

        std::cout << "  Science: ";
        for(auto [s, c] : p.scienceSymbols) {
            if(c > 0 && s != ScienceSymbol::NONE)
                std::cout << getScienceSymbolName(s) << " ";
        }
        std::cout << "\n";

        std::cout << "  Wonders: \n";
        int wCnt = 0;
        for(auto w : p.builtWonders) {
            std::cout << "    [V] " << std::left << std::setw(25) << w->name;
            if(++wCnt % 2 == 0) std::cout << "\n";
        }
        for(auto w : p.unbuiltWonders) {
            std::cout << "    [ ] " << std::left << std::setw(25) << w->name;
            if(++wCnt % 2 == 0) std::cout << "\n";
        }
        std::cout << "\n";
        std::cout << "└" << border << "┘\n";
    }

    void GameView::renderGame(const GameModel& model) {
        clearScreen();

        std::string ageInfo = "7 WONDERS DUEL - Age " + std::to_string(model.currentAge);
        int remCards = model.getRemainingCardCount();
        std::cout << std::string(80, '=') << "\n";
        std::cout << "  " << std::left << std::setw(40) << ageInfo << "Remaining Cards: " << remCards << "\n";
        std::cout << std::string(80, '=') << "\n\n";

        // Military
        int pos = model.board->militaryTrack.position;
        std::cout << "[MILITARY]  P1 ";
        for(int i=-9; i<=9; ++i) {
            if (i == pos) std::cout << "O";
            else if (i==0) std::cout << "|";
            else std::cout << "-";
        }
        std::cout << " P2  (Val: " << pos << ")\n";

        // Tokens (Name Only)
        std::cout << "[TOKENS]    ";
        for(auto t : model.board->availableProgressTokens) {
            std::cout << "[" << getTokenName(t) << "] ";
        }
        std::cout << "\n\n";

        // Players
        renderPlayerBox(*model.players[0], model.currentPlayerIndex == 0, *model.players[1]);
        renderPlayerBox(*model.players[1], model.currentPlayerIndex == 1, *model.players[0]);

        std::cout << std::string(80, '=') << "\n";

        // Logs
        if (!model.gameLog.empty()) {
            size_t start = model.gameLog.size() > 3 ? model.gameLog.size() - 3 : 0;
            for (size_t i = start; i < model.gameLog.size(); ++i) {
                std::cout << " > " << model.gameLog[i] << "\n";
            }
        }
    }

    // --- 字符串查找辅助 ---
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

    Action GameView::promptHumanAction(const GameModel& model) {
        renderGame(model);

        Action act;
        act.type = static_cast<ActionType>(-1);

        // 1. 轮抽
        if (model.currentAge == 0) {
            std::cout << "\n--- WONDER DRAFT ---\n";
            int idx = 1;
            for(auto w : model.draftPool) {
                std::cout << "[" << idx++ << "] " << w->name << "\n";
            }
            std::cout << "Enter number [1-4]: ";
            int c; std::cin >> c;
            if (c >= 1 && c <= model.draftPool.size()) {
                act.type = ActionType::DRAFT_WONDER;
                act.targetWonderId = model.draftPool[c-1]->id;
            }
            return act;
        }

        // 2. 主流程
        std::cout << "\n[" << model.getCurrentPlayer()->name << "]'s Turn - Available Cards:\n";
        auto avail = model.board->cardStructure.getAvailableCards();
        std::vector<Card*> visibleCards;
        std::vector<std::string> visibleNames;

        int idx = 1;
        for (auto slot : avail) {
            if (!slot->cardPtr) continue;
            Card* c = slot->cardPtr;
            visibleCards.push_back(c);
            visibleNames.push_back(c->name);

            std::cout << "[" << idx++ << "] " << std::left << std::setw(15) << c->name
                      << std::setw(8) << getTypeStr(c->type)
                      << " Cost: " << formatCost(c->cost) << " => ";
            for(auto& eff : c->effects) std::cout << eff->getDescription() << " ";
            std::cout << "\n";
        }

        std::cout << "\nCommands:\n";
        std::cout << "  build [name]        - Build card\n";
        std::cout << "  discard [name]      - Sell card\n";
        std::cout << "  wonder [card] [wdr] - Build wonder\n";
        std::cout << "  query [name]        - View Info\n";
        std::cout << "  detail [1/2]        - View player details\n";
        std::cout << "  log                 - View full log\n";
        std::cout << "> ";

        std::string line;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::getline(std::cin, line);

        std::stringstream ss(line);
        std::string cmd;
        ss >> cmd;

        std::string remainder;
        std::getline(ss, remainder);
        size_t first = remainder.find_first_not_of(' ');
        if (std::string::npos != first) remainder = remainder.substr(first);

        if (cmd == "build" || cmd == "discard") {
            int matchIdx = findMatch(visibleNames, remainder);
            if (matchIdx != -1) {
                act.type = (cmd == "build") ? ActionType::BUILD_CARD : ActionType::DISCARD_FOR_COINS;
                act.targetCardId = visibleCards[matchIdx]->id;
            } else {
                std::cout << "Card not found: " << remainder << "\n(Press Enter)";
                std::cin.get();
            }
        }
        else if (cmd == "query") {
            bool found = false;
            // Search Tokens
            std::vector<ProgressToken> allTokens = {
                ProgressToken::AGRICULTURE, ProgressToken::URBANISM, ProgressToken::STRATEGY,
                ProgressToken::THEOLOGY, ProgressToken::ECONOMY, ProgressToken::MASONRY,
                ProgressToken::ARCHITECTURE, ProgressToken::LAW, ProgressToken::MATHEMATICS,
                ProgressToken::PHILOSOPHY
            };
            for(auto t : allTokens) {
                std::string tName = getTokenName(t);
                if(findMatch({tName}, remainder) != -1) {
                    clearScreen(); renderTokenDetail(t); found = true; break;
                }
            }
            // Search Wonders
            if (!found) {
                for(const auto& w : model.allWonders) {
                    if(findMatch({w.name}, remainder) != -1) {
                        clearScreen(); renderWonderDetail(w); found = true; break;
                    }
                }
            }
            // Search Cards
            if (!found) {
                for(const auto& c : model.allCards) {
                    if(findMatch({c.name}, remainder) != -1) {
                        clearScreen(); renderCardDetail(c); found = true; break;
                    }
                }
            }
            if(!found) std::cout << "No matching Card, Wonder or Token found.\n";
            std::cout << "(Press Enter)"; std::cin.get();
        }
        else if (cmd == "wonder") {
            // Interactive mode for ease
            std::cout << "Enter Material Card Name: ";
            std::string cName; std::getline(std::cin, cName);
            int cIdx = findMatch(visibleNames, cName);

            if (cIdx != -1) {
                std::cout << "Enter Wonder Name: ";
                std::string wName; std::getline(std::cin, wName);

                auto& unbuilt = model.getCurrentPlayer()->unbuiltWonders;
                std::vector<std::string> wNames;
                for(auto w : unbuilt) wNames.push_back(w->name);

                int wIdx = findMatch(wNames, wName);
                if (wIdx != -1) {
                    act.type = ActionType::BUILD_WONDER;
                    act.targetCardId = visibleCards[cIdx]->id;
                    act.targetWonderId = unbuilt[wIdx]->id;
                } else { std::cout << "Wonder not found.\n(Press Enter)"; std::cin.get(); }
            } else { std::cout << "Card not found.\n(Press Enter)"; std::cin.get(); }
        }
        else if (cmd == "detail") {
            if (remainder == "1") {
                renderPlayerDetailFull(*model.players[0], *model.players[1]);
            } else if (remainder == "2") {
                renderPlayerDetailFull(*model.players[1], *model.players[0]);
            }
        }
        else if (cmd == "log") {
            clearScreen();
            std::cout << "FULL LOG:\n";
            for(auto& l : model.gameLog) std::cout << l << "\n";
            std::cout << "\n(Press Enter)"; std::cin.get();
        }

        return act;
    }

    // 占位
    void GameView::renderMainMenu() { clearScreen(); std::cout<<"MENU...\n"; }
    void GameView::printMessage(const std::string& msg) { std::cout << msg << "\n"; }
    void GameView::printError(const std::string& msg) { std::cout << "ERR: " << msg << "\n"; }
    void GameView::printTurnInfo(const Player* p) {}
    std::string GameView::getCardColorCode(CardType t) { return getTypeStr(t); }
    std::string GameView::resourceToString(ResourceType r) { return ""; }
    void GameView::renderPlayer(const Player& p, bool) {}
    void GameView::renderBoard(const GameModel& m) {}
}