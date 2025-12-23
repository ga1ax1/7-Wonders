#include "InputManager.h"
#include "GameView.h"
#include <iostream>
#include <sstream>
#include <algorithm>

namespace SevenWondersDuel {

    int InputManager::parseId(const std::string& input, char prefix) {
        if (input.empty()) return -1;
        std::string numPart = input;
        if (toupper(input[0]) == prefix && input.size() > 1) numPart = input.substr(1);
        try { return std::stoi(numPart); } catch (...) { return -1; }
    }

    Action InputManager::promptHumanAction(GameView& view, const GameModel& model, GameState state) {
        Action act;
        act.type = static_cast<ActionType>(-1);

        while (true) {
            m_ctx.clear();
            view.renderGame(model, state, m_ctx, m_lastError);

            std::cout << "\n " << model.getCurrentPlayer()->getName() << " > ";

            std::string line;
            if (!std::getline(std::cin, line)) { act.type = ActionType::DISCARD_FOR_COINS; return act; }
            if (line.empty()) continue;

            clearLastError();
            std::stringstream ss(line);
            std::string cmd; ss >> cmd;
            std::string arg1, arg2; ss >> arg1 >> arg2;

            if (cmd == "log") { view.renderFullLog(model.getGameLog()); continue; }

            if (cmd == "detail") {
                int pIdx = -1;
                if (arg1 == "1") pIdx = 0;
                else if (arg1 == "2") pIdx = 1;

                if (pIdx != -1) {
                    view.renderPlayerDetailFull(*model.getPlayers()[pIdx], *model.getPlayers()[1-pIdx], *model.getBoard());
                } else {
                    view.renderPlayerDetailFull(*model.getCurrentPlayer(), *model.getOpponent(), *model.getBoard());
                }
                continue;
            }

            if (cmd == "info") {
                int cId = parseId(arg1, 'C'); int wId = parseId(arg1, 'W'); int sId = parseId(arg1, 'S'); int tId = parseId(arg1, 'T'); int dId = parseId(arg1, 'D');

                bool found = false;
                if (!found && cId!=-1 && m_ctx.cardIdMap.count(cId)) {
                    const Card* c = model.findCardById(m_ctx.cardIdMap[cId]);
                    if (c) { view.renderCardDetail(*c); found = true; }
                }
                if (!found && wId!=-1 && m_ctx.wonderIdMap.count(wId)) {
                    const Wonder* w = model.findWonderById(m_ctx.wonderIdMap[wId]);
                    if (w) { view.renderWonderDetail(*w); found = true; }
                }
                if (!found && sId!=-1) {
                    if (m_ctx.tokenIdMap.count(sId)) { view.renderTokenDetail(m_ctx.tokenIdMap[sId]); found=true; }
                    else if (m_ctx.boxTokenIdMap.count(sId)) { view.renderTokenDetail(m_ctx.boxTokenIdMap[sId]); found=true; }
                }
                if (!found && tId!=-1 && m_ctx.oppCardIdMap.count(tId)) {
                    const Card* c = model.findCardById(m_ctx.oppCardIdMap[tId]);
                    if (c) { view.renderCardDetail(*c); found = true; }
                }

                if (!found) setLastError("ID not found or not visible in current context.");
                continue;
            }

            if (state == GameState::WONDER_DRAFT_PHASE_1 || state == GameState::WONDER_DRAFT_PHASE_2) {
                if (cmd == "pick") {
                    int idx = parseId(arg1, ' ');
                    if (idx >= 1 && idx <= (int)model.getDraftPool().size()) {
                        act.type = ActionType::DRAFT_WONDER;
                        act.targetWonderId = model.getDraftPool()[idx-1]->getId();
                        return act;
                    } else setLastError("Invalid index.");
                } else setLastError("Use 'pick <N>'.");
            }
            else if (state == GameState::WAITING_FOR_TOKEN_SELECTION_PAIR || state == GameState::WAITING_FOR_TOKEN_SELECTION_LIB) {
                bool isBox = (state == GameState::WAITING_FOR_TOKEN_SELECTION_LIB);
                if (cmd == "select") {
                    int id = parseId(arg1, 'S');
                    auto& map = isBox ? m_ctx.boxTokenIdMap : m_ctx.tokenIdMap;
                    if (map.count(id)) {
                        act.type = ActionType::SELECT_PROGRESS_TOKEN;
                        act.selectedToken = map[id];
                        return act;
                    } else setLastError("Invalid Token ID (S1, S2...). ");
                } else setLastError("Use 'select <ID>'.");
            }
            else if (state == GameState::WAITING_FOR_DESTRUCTION) {
                if (cmd == "destroy") {
                    int id = parseId(arg1, 'T');
                    if (m_ctx.oppCardIdMap.count(id)) {
                        act.type = ActionType::SELECT_DESTRUCTION;
                        act.targetCardId = m_ctx.oppCardIdMap[id];
                        return act;
                    } else setLastError("Invalid Target ID (T1, T2...). ");
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
                    if (m_ctx.discardIdMap.count(id)) {
                        act.type = ActionType::SELECT_FROM_DISCARD;
                        act.targetCardId = m_ctx.discardIdMap[id];
                        return act;
                    } else setLastError("Invalid Discard ID (D1...). ");
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
                    if (m_ctx.cardIdMap.count(id)) {
                        act.type = (cmd == "build") ? ActionType::BUILD_CARD : ActionType::DISCARD_FOR_COINS;
                        act.targetCardId = m_ctx.cardIdMap[id];
                        return act;
                    } else setLastError("Invalid Card ID (C1...). ");
                }
                else if (cmd == "wonder") {
                    int cId = parseId(arg1, 'C');
                    int wId = parseId(arg2, 'W');
                    if (m_ctx.cardIdMap.count(cId) && m_ctx.wonderIdMap.count(wId)) {
                        act.type = ActionType::BUILD_WONDER;
                        act.targetCardId = m_ctx.cardIdMap[cId];
                        act.targetWonderId = m_ctx.wonderIdMap[wId];
                        return act;
                    } else setLastError("Format: wonder C<ID> W<ID>");
                }
                else if (cmd == "pile") { view.renderDiscardPile(model.getBoard()->getDiscardPile()); }
                else setLastError("Unknown command.");
            }
        }
    }

}
