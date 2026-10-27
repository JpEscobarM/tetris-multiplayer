#pragma once
/*
 * Logica do Tetris. NAO depende de SDL nem de rede: so regras do jogo.
 * Assim da para testar sozinha (tests/game_test.cpp) e o cliente so "liga os fios".
 */
#include <cstdint>
#include <deque>
#include <random>
#include <utility>
#include <vector>
#include "types.h"

namespace game {

constexpr int COLS = BOARD_COLS;
constexpr int ROWS = BOARD_ROWS;
constexpr int NUM_PIECES = 7;   // I, O, T, S, Z, J, L  (tipos 0..6)
constexpr int GARBAGE = 8;      // valor da celula de linha de lixo (pecas usam 1..7)

/** Posicao (relativa a origem da peca) dos 4 quadradinhos de uma peca. */
struct Cells { int x[4]; int y[4]; };

/** Quadradinhos da peca `type` (0..6) na rotacao `rot` (0..3). */
const Cells& piece_cells(int type, int rot);

/**
 * Gerador de pecas "7-bag" a partir de uma semente. Mesma semente => mesma sequencia
 * em qualquer maquina (usa so a saida do mt19937, que e padronizada).
 */
class PieceBag {
public:
    explicit PieceBag(uint32_t seed = 1) { reset(seed); }
    void reset(uint32_t seed);
    int next();               // retira a proxima peca
    int peek(int i = 0);      // espia a i-esima proxima peca, sem retirar
private:
    void fill(size_t n);
    std::mt19937 rng_;
    std::deque<int> queue_;
};

struct Piece { int type = 0, rot = 0, x = 0, y = 0; };

class Game {
public:
    Game() { start(1); pause(); }

    /** Zera tudo e comeca uma partida com a semente dada. */
    void start(uint32_t seed);
    /** Congela o jogo (usado na contagem regressiva e depois do fim). */
    void pause() { active_ = false; }
    void resume() { active_ = true; }
    bool active() const { return active_ && !over_; }
    bool over() const { return over_; }

    // ---- acoes do jogador (retornam true se aconteceu) ----
    bool move(int dx);          // -1 esquerda, +1 direita
    bool rotate(int dir);       // +1 horario, -1 anti-horario (com "wall kick" simples)
    bool soft_drop();           // desce 1 linha (+1 ponto); se nao der, trava a peca
    void hard_drop();           // cai direto e trava (+2 pontos por linha)
    void update(int dt_ms);     // gravidade: chame todo quadro com o tempo decorrido

    // ---- multiplayer ----
    void receive_garbage(int lines, int holeCol);  // fica pendente ate a proxima peca travar
    int take_attack();                             // linhas de lixo a enviar ao adversario (zera)

    // ---- consulta (para desenhar / bot) ----
    int cell(int x, int y) const { return board_[y][x]; }   // so o tabuleiro, sem a peca atual
    bool has_piece() const { return hasPiece_; }
    const Piece& current() const { return cur_; }
    int ghost_y() const;                                    // y onde a peca pousaria
    bool collides(int type, int rot, int x, int y) const;
    int next_type(int i = 0) { return bag_.peek(i); }
    int score() const { return score_; }
    int lines() const { return lines_; }
    int level() const { return lines_ / 10; }
    int pending_garbage() const;
    uint32_t pieces_spawned() const { return spawned_; }
    void snapshot(StatePayload& out) const;                 // tabuleiro + peca atual + pontos

private:
    void spawn();
    void lock_piece();
    void apply_garbage();
    int gravity_interval() const;

    uint8_t board_[ROWS][COLS];
    Piece cur_;
    bool hasPiece_ = false, over_ = false, active_ = false;
    PieceBag bag_;
    int score_ = 0, lines_ = 0, attackOut_ = 0, gravityAcc_ = 0;
    uint32_t spawned_ = 0;
    std::vector<std::pair<int, int>> pending_;   // (linhas, coluna do buraco)
};

}  // namespace game
