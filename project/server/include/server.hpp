#pragma once
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <pthread.h>
#include <random>
#include <string>
#include "types.h"

/*
 * Servidor do Tetris. Responsabilidades (pedidas pelo professor):
 *   1. gerar a semente aleatoria que torna a sequencia de pecas igual para os dois;
 *   2. intermediar os ataques (linhas de lixo) entre os jogadores;
 *   3. monitorar o Game Over e encerrar a partida para os dois ao mesmo tempo.
 *
 * Threads:
 *   - connection_announcement : anuncia o servidor por broadcast UDP enquanto houver vaga
 *   - accept_connections      : aceita ate 2 jogadores e cria uma thread handleClients para cada
 *   - handleClients (1 por jogador): le as mensagens do jogador e executa o que ele pediu
 */
namespace server {

    typedef struct
    {
        int socket;
        int id;   // 0 ou 1: qual jogador e este
    } SocketParam_t;

    inline constexpr unsigned short BROADCAST_PORT = 5001;   // UDP: anuncio do servidor
    inline constexpr unsigned short TCP_PORT = 5002;         // TCP: jogo
    inline constexpr unsigned short TIME_TO_CHECK = 1;       // segundos entre anuncios
    inline constexpr int CONNECTION_LIMIT = 2;               // numero de jogadores

    // ---- Estado compartilhado: TODO acesso exige 'lock' ----
    inline int socketClients[CONNECTION_LIMIT] = {-1, -1};   // -1 = vaga livre
    inline int connectedClients = 0;
    inline bool joined[CONNECTION_LIMIT] = {false, false};   // quem ja mandou MSG_JOIN
    inline bool gameRunning = false;
    inline std::mt19937 rng{std::random_device{}()};

    inline int socketUDP = -1;
    inline int socketTCP = -1;
    inline sockaddr_in enderecoServidor;
    inline sockaddr_in enderecoBroadcast;

    inline pthread_t connectionThreads[2];
    inline pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

    bool init_server();
    bool configure_udp_socket();
    bool configure_tcp_socket();
    bool initConnectionThreads();
    void* connection_announcement(void *args);
    void* accept_connections(void *args);
    void* handleClients(void *args);
}
