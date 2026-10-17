cmake_minimum_required(VERSION 3.16)
project(tetris)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(tetris_client
client/src/main.cpp
client/src/client.cpp)

add_executable(tetris_server
        server/src/main.cpp
        server/src/server.cpp
)



target_include_directories( tetris_server PRIVATE server/include)

target_include_directories(tetris_client PRIVATE client/include)