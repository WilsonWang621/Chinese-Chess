cmake_minimum_required(VERSION 4.2)
project(Chess)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(Chess
        main.cpp
        src/Board.cpp
        src/Game.cpp
        src/SimpleAI.cpp
        src/Types.cpp
)

target_include_directories(Chess PRIVATE include)
