#ifndef TETRIS_TYPES_H
#define TETRIS_TYPES_H

#include <cstdint>

enum MessageType : int32_t {
    MSG_JOIN       = 1,  // cliente -> servidor: "estou pronto"
    MSG_GAME_STATE = 3,  // cliente -> servidor -> outro cliente: foto do tabuleiro + pontos
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

// Conteudo (payload) de cada tipo. Sempre copiar com memcpy.
typedef struct { int32_t seed; int32_t playerId; } StartPayload;
typedef struct { int32_t lines; }                  AttackPayload;
typedef struct { int32_t winnerId; }               EndPayload;

#endif