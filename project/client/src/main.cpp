#include <SDL.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "bot.hpp"
#include "client.hpp"
#include "game.hpp"
#include "render.hpp"

/*
 * Uso:
 *   ./tetris_client                 descobre o servidor sozinho (broadcast UDP)
 *   ./tetris_client 192.168.0.10    conecta direto no IP informado
 *   ./tetris_client --bot           joga sozinho (para testes)
 */
int main(int argc, char *argv[])
{
    const char *serverIp = nullptr;
    bool useBot = false;
    double botSkill = 0.9;

    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "--bot", 5) == 0) {
            useBot = true;
            if (argv[i][5] == '=') botSkill = atof(argv[i] + 6);
        } else {
            serverIp = argv[i];
        }
    }

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "ERRO AO INICIAR SDL: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("Tetris Multiplayer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          render::WIN_W, render::WIN_H, SDL_WINDOW_RESIZABLE);
    if (!window) {
        fprintf(stderr, "ERRO AO CRIAR JANELA: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (!renderer) {
        fprintf(stderr, "ERRO AO CRIAR RENDERER: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    SDL_RenderSetLogicalSize(renderer, render::WIN_W, render::WIN_H);   // a janela pode ser redimensionada

    client::start_connection(serverIp);

    // ------------------------------------------------------------ estado da partida
    game::Game game;
    bot::Bot botPlayer(botSkill);
    StatePayload opponent;
    memset(&opponent, 0, sizeof(opponent));
    bool haveOpponent = false;
    render::Screen screen = render::Screen::Connecting;
    int myId = -1;
    bool won = false;
    Uint32 countdownEnd = 0, lastStateSent = 0, last = SDL_GetTicks();
    bool running = true;
    int botExitCode = 0;

    // Envia a "foto" do meu tabuleiro para o adversario (via servidor).
    auto send_state = [&]() {
        StatePayload sp;
        game.snapshot(sp);
        client::send_message(MSG_GAME_STATE, &sp, sizeof(sp));
    };

    // Depois de qualquer acao: manda ataque, e avisa o servidor se eu perdi.
    auto after_action = [&]() {
        if (screen != render::Screen::Playing) return;

        int attack = game.take_attack();
        if (attack > 0) {
            AttackPayload ap{attack, 0};
            client::send_message(MSG_ATTACK, &ap, sizeof(ap));
            printf("[cliente] ataque enviado: %d linha(s) de lixo\n", attack);
        }
        if (game.over()) {
            printf("[cliente] game over (placar %d)\n", game.score());
            send_state();
            client::send_message(MSG_GAME_OVER, nullptr, 0);
            screen = render::Screen::WaitingEnd;
        }
    };

    int heldDir = 0, dasTimer = 0, softTimer = 0;
    bool dasRepeating = false;

    while (running) {
        Uint32 now = SDL_GetTicks();
        int dt = (int)(now - last);
        last = now;

        // ------------------------------------------------------------------ teclado
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;
            if (e.type != SDL_KEYDOWN) continue;

            SDL_Keycode k = e.key.keysym.sym;
            if (k == SDLK_ESCAPE) running = false;

            if (screen == render::Screen::Playing && !useBot && !e.key.repeat) {
                if (k == SDLK_UP || k == SDLK_x) game.rotate(1);
                else if (k == SDLK_z) game.rotate(-1);
                else if (k == SDLK_SPACE) game.hard_drop();
            }
            if (screen == render::Screen::Finished && (k == SDLK_RETURN || k == SDLK_KP_ENTER)) {
                client::send_message(MSG_JOIN, nullptr, 0);
                haveOpponent = false;
                screen = render::Screen::Lobby;
            }
        }

        // ------------------------------------------------------------------- conexao
        if (client::connection_lost() && screen != render::Screen::Disconnected) {
            printf("[cliente] conexao com o servidor perdida\n");
            screen = render::Screen::Disconnected;
            if (useBot) { botExitCode = 2; running = false; }
        }
        if (screen == render::Screen::Connecting && client::is_connected()) {
            client::send_message(MSG_JOIN, nullptr, 0);
            screen = render::Screen::Lobby;
        }

        // ------------------------------------------------- mensagens vindas do servidor
        Message m;
        while (client::poll_message(m)) {
            switch (m.option) {
                case MSG_START: {
                    StartPayload sp;
                    memcpy(&sp, m.payload, sizeof(sp));
                    myId = sp.playerId;
                    game.start((uint32_t)sp.seed);
                    game.pause();                                 // congelado ate acabar a contagem
                    memset(&opponent, 0, sizeof(opponent));
                    haveOpponent = false;
                    countdownEnd = now + 3000;
                    screen = render::Screen::Countdown;
                    heldDir = 0;
                    printf("[cliente] partida iniciada: seed=%d, voce e o jogador %d\n", sp.seed, myId);
                    char title[64];
                    snprintf(title, sizeof(title), "Tetris Multiplayer - Jogador %d", myId + 1);
                    SDL_SetWindowTitle(window, title);
                    break;
                }
                case MSG_ATTACK: {
                    AttackPayload ap;
                    memcpy(&ap, m.payload, sizeof(ap));
                    if (screen == render::Screen::Playing) {
                        game.receive_garbage(ap.lines, ap.holeCol);
                        printf("[cliente] lixo recebido: %d linha(s), buraco na coluna %d\n", ap.lines, ap.holeCol);
                    }
                    break;
                }
                case MSG_GAME_STATE:
                    if (screen == render::Screen::Countdown || screen == render::Screen::Playing ||
                        screen == render::Screen::WaitingEnd) {
                        memcpy(&opponent, m.payload, sizeof(opponent));
                        haveOpponent = true;
                    }
                    break;
                case MSG_GAME_END: {
                    EndPayload ep;
                    memcpy(&ep, m.payload, sizeof(ep));
                    won = (ep.winnerId == myId);
                    game.pause();
                    screen = render::Screen::Finished;
                    printf("[cliente] fim da partida: voce %s (placar %d)\n", won ? "VENCEU" : "PERDEU", game.score());
                    if (useBot) {
                        botExitCode = won ? 0 : 1;
                        running = false;
                    }
                    break;
                }
                default:
                    break;
            }
        }

        // ------------------------------------------------------------ atualizacao do jogo
        int countdown = 0;
        if (screen == render::Screen::Countdown) {
            if (now >= countdownEnd) {
                game.resume();
                screen = render::Screen::Playing;
                lastStateSent = now;
                send_state();
            } else {
                countdown = (int)((countdownEnd - now + 999) / 1000);
            }
        }

        if (screen == render::Screen::Playing) {
            if (useBot) {
                botPlayer.update(game, dt);
            } else {
                // esquerda/direita: anda 1 casa, e depois de um tempo segurando repete rapido
                const Uint8 *keys = SDL_GetKeyboardState(NULL);
                int dir = (keys[SDL_SCANCODE_RIGHT] ? 1 : 0) - (keys[SDL_SCANCODE_LEFT] ? 1 : 0);
                if (dir != heldDir) {
                    heldDir = dir;
                    dasTimer = 0;
                    dasRepeating = false;
                    if (dir != 0) game.move(dir);
                } else if (dir != 0) {
                    dasTimer += dt;
                    if (!dasRepeating && dasTimer >= 170) { dasRepeating = true; dasTimer = 0; game.move(dir); }
                    while (dasRepeating && dasTimer >= 50) { dasTimer -= 50; game.move(dir); }
                }
                // seta para baixo: desce rapido
                if (keys[SDL_SCANCODE_DOWN]) {
                    softTimer += dt;
                    while (softTimer >= 40) { softTimer -= 40; game.soft_drop(); }
                } else {
                    softTimer = 0;
                }
            }
            game.update(dt);
            after_action();

            if (screen == render::Screen::Playing && now - lastStateSent >= 100) {   // 10 fotos por segundo
                lastStateSent = now;
                send_state();
            }
        }

        // ---------------------------------------------------------------------- desenho
        render::View view;
        view.screen = screen;
        view.game = &game;
        view.opponent = haveOpponent ? &opponent : nullptr;
        view.countdown = countdown;
        view.won = won;
        view.playerId = myId;
        render::draw(renderer, view);
        SDL_RenderPresent(renderer);
        SDL_Delay(8);
    }

    client::shutdown_client();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return botExitCode;
}
