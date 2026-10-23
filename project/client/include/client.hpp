#pragma once
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstdio>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <iostream>
#include <pthread.h>
#include "types.h"

namespace client
{
    /**
     * @brief Porta UDP usada para receber os anúncios do servidor.
     */
    inline constexpr unsigned short BROADCAST_PORT = 5001;

    /**
     * @brief Descritor do socket UDP usado na descoberta do servidor.
     */
    inline int socketUDP = -1;

    /**
     * @brief Descritor do socket TCP usado na comunicação com o servidor.
     */
    inline int socketTCP = -1;

    /**
     * @brief Endereço local usado para vincular o socket UDP.
     */
    inline sockaddr_in enderecoClienteBroadcast;

    /**
     * @brief Endereço do servidor obtido durante a descoberta.
     * @note Inicialmente contém o endereço de origem do datagrama UDP.
     *       A porta é substituída pela porta TCP antes da conexão.
     */
    inline sockaddr_in enderecoServer;

    /**
     * @brief Identificador da thread de descoberta e conexão com o servidor.
     */
    inline pthread_t connectionThread;

    /**
     * @brief Identificador da thread que produz eventos a partir da entrada padrão.
     */
    inline pthread_t eventProducerThread;

    /**
     * @brief Identificador da thread que consome eventos e os envia ao servidor.
     */
    inline pthread_t eventConsumerThread;

    /**
     * @brief Quantidade máxima de eventos armazenados na fila circular.
     */
    inline constexpr int EVENT_POOL_BUFFER_SIZE = 1024;

    /**
     * @brief Evento textual armazenado na fila para envio ao servidor.
     */
    typedef struct
    {
        /**
         * @brief Buffer da mensagem textual.
         * @note Deve conter uma string terminada em '\0' antes do envio.
         */
        char buffer[1024];

    } Event;

    /**
     * @brief Armazenamento da fila circular de eventos pendentes de envio.
     * @note O acesso deve ser protegido por event_lock.
     */
    inline Message eventPool[EVENT_POOL_BUFFER_SIZE];

    /**
     * @brief Índices de escrita e leitura da fila circular.
     * @details writeIndex indica a próxima posição de inserção;
     *          readIndex indica a próxima posição de retirada.
     * @note O acesso deve ser protegido por event_lock.
     */
    inline int writeIndex = 0, readIndex = 0;

    /**
     * @brief Quantidade de eventos atualmente armazenados na fila.
     * @note O acesso deve ser protegido por event_lock.
     */
    inline int eventPoolSize = 0;

    /**
     * @brief Mutex que protege a fila, seus índices e a quantidade de eventos.
     */
    inline pthread_mutex_t event_lock = PTHREAD_MUTEX_INITIALIZER;

    /**
     * @brief Condição usada pelo consumidor para aguardar eventos na fila.
     * @note Sinalizada pelo produtor após inserir um evento.
     *       A espera deve verificar eventPoolSize sob event_lock.
     */
    inline pthread_cond_t hasEvent = PTHREAD_COND_INITIALIZER;

    /**
     * @brief Condição usada pelo produtor para aguardar espaço na fila.
     * @note Sinalizada pelo consumidor após retirar um evento.
     *       A espera deve verificar eventPoolSize sob event_lock.
     */
    inline pthread_cond_t hasSpace = PTHREAD_COND_INITIALIZER;

    /**
     * @brief Configura a recepção UDP e aguarda a rotina de descoberta e conexão.
     * @return false se a configuração UDP falhar; true após a rotina de conexão terminar.
     * @note O retorno true não garante que a conexão TCP foi estabelecida.
     */
    bool init_client();

    /**
     * @brief Cria o socket UDP e o vincula à porta BROADCAST_PORT nas interfaces IPv4 locais.
     * @return true se a criação e o bind forem bem-sucedidos; false caso contrário.
     */
    bool configure_udp_socket();

    /**
     * @brief Aguarda um datagrama UDP, extrai a porta anunciada e tenta conectar-se via TCP.
     * @param args Parâmetro da thread, não utilizado.
     * @note O IP do servidor é obtido pelo endereço de origem do datagrama.
     * @note Encerra a thread com resultado NULL após a tentativa de conexão.
     */
    void* listen_server_announcement(void *args);

    /**
     * @brief Cria a thread de descoberta do servidor e aguarda seu término.
     * @note Bloqueia a thread chamadora enquanto a descoberta e a tentativa de conexão são executadas.
     */
    void initConnectionThreads();

    /**
     * @brief Cria o socket TCP e tenta conectar-se ao IP armazenado em enderecoServer.
     * @param port Porta TCP do servidor, em ordem de bytes do host.
     * @return true se a conexão for estabelecida; false caso contrário.
     * @pre enderecoServer deve conter o IP obtido durante a descoberta.
     */
    bool connectTCP(unsigned short port);

    /**
     * @brief Converte o segundo campo separado por '|' em um número de porta.
     * @param[in,out] message String modificável e terminada em '\0', contendo o anúncio.
     * @return Porta entre 1 e 65535, ou 0 se a extração falhar ou o valor for zero.
     * @note Modifica a string recebida e não valida o conteúdo do primeiro campo.
     */
    unsigned short extract_port(char *message);

    /**
     * @brief Retira eventos em ordem FIFO e envia suas mensagens pelo socket TCP.
     * @param args Parâmetro da thread, não utilizado.
     * @return NULL se ocorrer uma falha de envio não recuperada ou um envio sem progresso.
     * @note Aguarda hasEvent enquanto a fila estiver vazia.
     * @note Inclui o terminador '\0' no envio e trata envios parciais.
     */
    void* consumer_client_event_pool(void *args);

    /**
     * @brief Lê textos da entrada padrão e os insere na fila circular de eventos.
     * @param args Parâmetro da thread, não utilizado.
     * @note Remove o '\n' quando presente e aguarda hasSpace enquanto a fila estiver cheia.
     * @note Encerra a thread com resultado NULL quando a leitura falhar ou atingir EOF.
     */
    void* producer_client_event_pool(void *args);

    /**
     * @brief Cria as threads produtora e consumidora de eventos e aguarda o término de ambas.
     * @pre O socket TCP deve estar conectado para permitir o envio dos eventos.
     * @note Encerra o processo com EXIT_FAILURE se a criação de alguma thread falhar.
     * @note Pode bloquear indefinidamente, pois ainda não há encerramento coordenado das threads.
     */
    void init_event_pool();
}