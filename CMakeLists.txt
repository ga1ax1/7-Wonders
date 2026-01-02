cmake_minimum_required(VERSION 3.10)
project(SevenWondersDuel)

set(CMAKE_CXX_STANDARD 17)

# Headers
include_directories(include)

# Source files
set(SOURCES
    src/Agent.cpp
    src/Board.cpp
    src/Card.cpp
    src/CardBuilder.cpp
    src/EffectSystem.cpp
    src/GameCommands.cpp
    src/GameController.cpp
    src/GameFactory.cpp
    src/GameModel.cpp
    src/GameStateLogic.cpp
    src/GameView.cpp
    src/Global.cpp
    src/InputManager.cpp
    src/Player.cpp
    src/RenderContext.cpp
    src/RulesEngine.cpp
    src/ScoringManager.cpp
)

# Executable
add_executable(SevenWondersDuel main.cpp ${SOURCES})

# Include paths for the target
target_include_directories(SevenWondersDuel PUBLIC include)