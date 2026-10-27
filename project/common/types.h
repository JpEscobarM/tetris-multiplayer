#ifndef TETRIS_TYPES_H
#define TETRIS_TYPES_H

#include <cstdint>

/*
 * PROTOCOLO (tudo trafega em TCP, sempre com mensagens de tamanho fixo = sizeof(Message)):
 *
 *   cliente -> servidor            servidor -> cliente
 *   -------------------            -------------------
 *   MSG_JOIN       "estou pronto"  MSG_START     {seed, playerId}  (os dois ao mesmo tempo)
 *   MSG_GAME_STATE foto do jogo    MSG_GAME_STATE (repasse da foto do adversario)
 *   MSG_ATTACK     {lines}         MSG_ATTACK     {lines, holeCol} (so para o adversario)
 *   MSG_GAME_OVER  "perdi"         MSG_GAME_END   {winnerId}       (os dois ao mesmo tempo)
 *   MSG_DISCONNECT "estou saindo"
 */
enum MessageType : int32_t {
    MSG_JOIN       = 1,  // cliente -> servidor: "estou pronto para jogar"
    MSG_GAME_STATE = 3,  // cliente -> servidor -> outro cliente: tabuleiro + pontos
    MSG_CHAT       = 4,  // so para depuracao
    MSG_DISCONNECT = 5,  // cliente -> servidor
    MSG_START      = 6,  // servidor -> clientes: semente + id do jogador
    MSG_ATTACK     = 7,  // cliente -> servidor -> outro cliente: linhas de lixo
    MSG_GAME_OVER  = 8,  // cliente -> servidor: "perdi"
    MSG_GAME_END   = 9   // servidor -> clientes: quem venceu
};

#define PAYLOAD_SIZE 256

typedef struct {
    MessageType option;
    char payload[PAYLOAD_SIZE];
} Message;

#define BOARD_COLS 10
#define BOARD_ROWS 20

// Conteudo (payload) de cada tipo. Sempre copiar com memcpy.
typedef struct { int32_t seed; int32_t playerId; } StartPayload;

// Cliente -> servidor: lines = linhas de lixo (1 a 4); holeCol e ignorado.
// Servidor -> cliente: o servidor sorteia holeCol (coluna do buraco, 0 a 9).
typedef struct { int32_t lines; int32_t holeCol; } AttackPayload;

typedef struct { int32_t winnerId; } EndPayload;

// Foto do tabuleiro: 0 = vazio, 1..7 = peca, 8 = lixo. Ja inclui a peca que esta caindo.
typedef struct {
    uint8_t cells[BOARD_COLS * BOARD_ROWS];
    int32_t score;
    int32_t lines;
} StatePayload;

static_assert(sizeof(StatePayload) <= PAYLOAD_SIZE, "StatePayload precisa caber no payload");

#endif
