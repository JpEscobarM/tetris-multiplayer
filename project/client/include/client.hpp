#pragma once
/*
 * Rede do cliente.
 *
 *   thread de conexao  : descobre o servidor (broadcast UDP) e conecta por TCP
 *   thread de envio    : retira mensagens da fila (produtor = jogo) e manda pelo TCP
 *   thread de recepcao : le mensagens do servidor e coloca na caixa de entrada
 *
 * O jogo (thread principal, SDL) so usa: start_connection, is_connected, connection_lost,
 * send_message, poll_message e shutdown_client. Nunca bloqueia na rede.
 */
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <atomic>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <pthread.h>
#include "types.h"

namespace client
{
    /** @brief Porta UDP usada para receber os anuncios do servidor. */
    inline constexpr unsigned short BROADCAST_PORT = 5001;

    /** @brief Porta TCP do servidor (usada quando o IP e informado manualmente). */
    inline constexpr unsigned short TCP_PORT = 5002;

    inline int socketUDP = -1;
    inline int socketTCP = -1;
    inline sockaddr_in enderecoClienteBroadcast;

    /** @brief Endereco do servidor (IP vem do datagrama UDP; a porta e trocada pela TCP). */
    inline sockaddr_in enderecoServer;

    inline pthread_t connectionThread;
    inline pthread_t eventConsumerThread;   // envio
    inline pthread_t receiverThread;        // recepcao

    // ---------------------------------------------------------------- fila de ENVIO
    inline constexpr int EVENT_POOL_BUFFER_SIZE = 1024;
    inline Message eventPool[EVENT_POOL_BUFFER_SIZE];
    inline int writeIndex = 0, readIndex = 0;
    inline int eventPoolSize = 0;
    inline pthread_mutex_t event_lock = PTHREAD_MUTEX_INITIALIZER;
    inline pthread_cond_t hasEvent = PTHREAD_COND_INITIALIZER;

    // ------------------------------------------------------- caixa de ENTRADA (recepcao)
    inline constexpr int INBOX_SIZE = 1024;
    inline Message inbox[INBOX_SIZE];
    inline int inHead = 0, inCount = 0;
    inline pthread_mutex_t inbox_lock = PTHREAD_MUTEX_INITIALIZER;

    // -------------------------------------------------------------------- estado
    inline std::atomic<bool> connected{false};
    inline std::atomic<bool> connectionLost{false};
    inline std::atomic<bool> stopping{false};
    inline bool connectionThreadStarted = false;
    inline bool ioThreadsStarted = false;
    inline char manualServerIp[64] = "";

    /**
     * @brief Inicia (em segundo plano) a conexao com o servidor.
     * @param serverIp IP do servidor, ou nullptr para descobrir por broadcast UDP.
     */
    void start_connection(const char *serverIp);

    /** @brief true depois que a conexao TCP foi estabelecida. */
    bool is_connected();

    /** @brief true se o servidor caiu / a conexao foi perdida. */
    bool connection_lost();

    /**
     * @brief Coloca uma mensagem na fila de envio (nao bloqueia).
     * @return false se nao ha conexao ou a fila esta cheia.
     */
    bool send_message(MessageType type, const void *data, size_t len);

    /** @brief Retira a proxima mensagem recebida do servidor, se houver (nao bloqueia). */
    bool poll_message(Message &out);

    /** @brief Avisa o servidor (MSG_DISCONNECT), encerra as threads e fecha os sockets. */
    void shutdown_client();

    // ------------------------------------------------------------- funcoes internas
    bool configure_udp_socket();
    unsigned short extract_port(char *message);
    bool connectTCP(unsigned short port);
    void *connection_routine(void *args);
    void *consumer_client_event_pool(void *args);
    void *receiver_routine(void *args);
}
