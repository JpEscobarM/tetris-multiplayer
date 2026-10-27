#pragma once
/* Desenho da tela com SDL2 (sem imagens e sem fontes externas: tudo e desenhado por codigo). */
#include <SDL.h>
#include "game.hpp"

namespace render {

constexpr int WIN_W = 900;
constexpr int WIN_H = 700;

enum class Screen {
    Connecting,    // procurando o servidor
    Lobby,         // conectado, esperando o adversario
    Countdown,     // 3, 2, 1...
    Playing,
    WaitingEnd,    // eu perdi e aviso o servidor, esperando MSG_GAME_END
    Finished,      // partida acabou (won diz quem ganhou)
    Disconnected   // conexao com o servidor caiu
};

struct View {
    Screen screen = Screen::Connecting;
    const game::Game* game = nullptr;          // meu jogo
    const StatePayload* opponent = nullptr;    // foto do adversario (pode ser nulo)
    int countdown = 0;                         // numero mostrado na contagem
    bool won = false;
    int playerId = -1;
};

/** Desenha um quadro completo. */
void draw(SDL_Renderer* r, const View& v);

/** Texto com fonte 5x7 embutida (maiusculas, digitos e alguns simbolos). */
void draw_text(SDL_Renderer* r, int x, int y, int scale, const char* text, SDL_Color color);
int text_width(const char* text, int scale);

}  // namespace render
