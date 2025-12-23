//
// Created by choyichi on 2025/12/23.
//

#ifndef SEVEN_WONDERS_DUEL_GAMECOMMANDS_H
#define SEVEN_WONDERS_DUEL_GAMECOMMANDS_H

#include "Global.h"
#include <memory>

namespace SevenWondersDuel {

    class GameController;

    class IGameCommand {
    public:
        virtual ~IGameCommand() = default;
        virtual void execute(GameController& controller) = 0;
    };

    class CommandFactory {
    public:
        static std::unique_ptr<IGameCommand> createCommand(const Action& action);
    };

    // --- Concrete Commands ---

    class DraftWonderCommand : public IGameCommand {
        std::string wonderId;
    public:
        explicit DraftWonderCommand(std::string id);
        void execute(GameController& controller) override;
    };

    class BuildCardCommand : public IGameCommand {
        std::string cardId;
    public:
        explicit BuildCardCommand(std::string id);
        void execute(GameController& controller) override;
    };

    class DiscardCardCommand : public IGameCommand {
        std::string cardId;
    public:
        explicit DiscardCardCommand(std::string id);
        void execute(GameController& controller) override;
    };

    class BuildWonderCommand : public IGameCommand {
        std::string cardId;
        std::string wonderId;
    public:
        BuildWonderCommand(std::string cid, std::string wid);
        void execute(GameController& controller) override;
    };

    class SelectProgressTokenCommand : public IGameCommand {
        ProgressToken token;
    public:
        explicit SelectProgressTokenCommand(ProgressToken t);
        void execute(GameController& controller) override;
    };

    class DestructionCommand : public IGameCommand {
        std::string targetId;
    public:
        explicit DestructionCommand(std::string tid);
        void execute(GameController& controller) override;
    };

    class SelectFromDiscardCommand : public IGameCommand {
        std::string cardId;
    public:
        explicit SelectFromDiscardCommand(std::string id);
        void execute(GameController& controller) override;
    };

    class ChooseStartingPlayerCommand : public IGameCommand {
        std::string targetId; // "ME" or "OPPONENT"
    public:
        explicit ChooseStartingPlayerCommand(std::string tid);
        void execute(GameController& controller) override;
    };

}

#endif // SEVEN_WONDERS_DUEL_GAMECOMMANDS_H
