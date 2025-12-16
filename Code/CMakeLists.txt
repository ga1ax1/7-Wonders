cmake_minimum_required(VERSION 4.0)
project(7-Wonders)

set(CMAKE_CXX_STANDARD 17)

add_executable(7-Wonders main.cpp
        Global.h
        EffectSystem.h
        Card.h
        Player.h
        Board.h
        GameController.h
        Player.cpp
        EffectSystem.cpp
        GameController.cpp
        Board.cpp
        DataLoader.cpp
        DataLoader.h
        TinyJson.h
        Agent.cpp
        Agent.h
        GameView.cpp
        GameView.h
)
