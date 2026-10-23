//
// Created by jmartins on 23/10/2026.
//

#ifndef TETRIS_TYPES_H
#define TETRIS_TYPES_H


enum MessageType {
    MSG_JOIN = 1,
    MSG_MOVE = 2,
    MSG_GAME_STATE = 3,
    MSG_CHAT = 4,
    MSG_DISCONNECT = 5
};

#define PAYLOAD_SIZE 256

typedef struct {
    MessageType option;
    char payload[PAYLOAD_SIZE];
} Message;

#endif //TETRIS_TYPES_H