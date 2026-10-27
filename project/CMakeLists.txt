cmake_minimum_required(VERSION 3.16)
project(tetris)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)   # pthread (Linux/WSL)
find_package(SDL2 REQUIRED)      # sudo apt install libsdl2-dev

# ------------------------------------------------------------------ servidor
add_executable(tetris_server
        server/src/main.cpp
        server/src/server.cpp)
target_include_directories(tetris_server PRIVATE server/include common)
target_link_libraries(tetris_server PRIVATE Threads::Threads)

# ------------------------------------------------------------------- cliente
add_executable(tetris_client
        client/src/main.cpp
        client/src/client.cpp
        client/src/game.cpp
        client/src/render.cpp
        client/src/bot.cpp)
target_include_directories(tetris_client PRIVATE client/include common)
target_link_libraries(tetris_client PRIVATE Threads::Threads SDL2::SDL2)

# ------------------------------------------- testes da logica do jogo (sem SDL/rede)
add_executable(tetris_tests
        tests/game_test.cpp
        client/src/game.cpp)
target_include_directories(tetris_tests PRIVATE client/include common)
