#pragma once
/* Jogador automatico (opcional): ./tetris_client --bot  |  --bot=0.4 (quanto menor, mais erra).
 * Serve para testar o jogo sozinho: abra um cliente normal e um com --bot. */
#include <cstdint>
#include <random>
#include "game.hpp"

namespace bot {

class Bot {
public:
    explicit Bot(double skill = 0.9, int stepMs = 45);
    void update(game::Game& g, int dtMs);   // chame todo quadro enquanto estiver jogando
private:
    void plan(game::Game& g);
    double skill_;
    int stepMs_, acc_ = 0;
    uint32_t lastSpawn_ = 0;
    int targetRot_ = 0, targetX_ = 0;
    std::mt19937 rng_;
};

}  // namespace bot
