#pragma once
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstdio>
#include <string>
#include <strings.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <pthread.h>

namespace server {

    /**
     * @brief Parâmetro usado para passar o socket de um cliente à thread de atendimento.
     */
    typedef struct
    {
        /**
         * @brief Descritor do socket TCP retornado por accept().
         */
        int socket;

    }SocketParam_t;

    /**
     * @brief Porta UDP de destino dos anúncios de descoberta do servidor.
     */
    inline constexpr unsigned short BROADCAST_PORT = 5001;

    /**
     * @brief Porta local usada para receber conexões TCP dos clientes.
     */
    inline constexpr unsigned short TCP_PORT = 5002;

    /**
     * @brief Intervalo, em segundos, entre os anúncios UDP.
     */
    inline constexpr unsigned short TIME_TO_CHECK = 3;

    /**
     * @brief Limite de atendimentos criados pela rotina de aceitação.
     * @note Também é usado como backlog em listen().
     * @note Não deve exceder a capacidade dos arrays de sockets e threads.
     */
    inline int CONNECTION_LIMIT = 2;

    /**
     * @brief Descritores dos sockets TCP aceitos para atendimento dos clientes.
     * @note A posição é marcada com -1 quando o atendimento é encerrado
     *       ou quando a criação da thread correspondente falha.
     */
    inline int socketCients[2];

    /**
     * @brief Contador de clientes atendidos pelo servidor.
     * @note As leituras e alterações devem ser protegidas por lock.
     */
    inline int connectedClients = 0;

    /**
     * @brief Descritor do socket UDP usado para enviar anúncios em broadcast.
     */
    inline int socketUDP = -1;

    /**
     * @brief Descritor do socket TCP usado para escutar e aceitar novas conexões.
     * @note A comunicação com cada cliente utiliza o socket retornado por accept().
     */
    inline int socketTCP = -1;

    /**
     * @brief Endereço IPv4 local usado no bind() do socket TCP.
     */
    inline sockaddr_in enderecoServidor;

    /**
     * @brief Endereço de destino dos anúncios UDP.
     * @details Utiliza o broadcast 255.255.255.255 e a porta BROADCAST_PORT.
     */
    inline sockaddr_in enderecoBroadcast;

    /**
     * @brief Identificadores das threads de anúncio e aceitação de conexões.
     * @details A posição 0 corresponde ao anúncio UDP;
     *          a posição 1 corresponde à aceitação TCP.
     */
    inline pthread_t connectionThreads[2];

    /**
     * @brief Identificadores das threads responsáveis pelo atendimento dos clientes.
     */
    inline pthread_t handleClientsThreads[2];

    /**
     * @brief Mutex usado para sincronizar o acesso ao estado compartilhado dos clientes.
     */
    inline pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

    /**
     * @brief Configura os sockets UDP e TCP e executa a inicialização das threads.
     * @return false se alguma rotina de configuração falhar;
     *         true após initConnectionThreads() retornar.
     * @note Aguarda as threads por meio de initConnectionThreads().
     */
    bool init_server();

    /**
     * @brief Cria o socket UDP e configura o destino dos anúncios em broadcast.
     * @return false se a criação do socket falhar; true caso contrário.
     * @note Não realiza bind() na porta BROADCAST_PORT, que é a porta de destino.
     * @note A implementação atual não verifica o retorno de setsockopt().
     */
    bool configure_udp_socket();

    /**
     * @brief Executa a rotina de anúncios UDP com a identificação e a porta TCP do servidor.
     * @param args Parâmetro da thread, não utilizado.
     * @details Envia mensagens no formato "TETRIS|porta" para enderecoBroadcast,
     *          com intervalos definidos por TIME_TO_CHECK.
     * @note A rotina consulta connectedClients para decidir quando parar os anúncios.
     * @note Encerra a thread com resultado NULL.
     */
    void* connection_announcement(void *args);

    /**
     * @brief Cria o socket TCP e o vincula à porta TCP_PORT nas interfaces IPv4 locais.
     * @return true se o bind() for bem-sucedido; false caso contrário.
     * @note listen() e accept() são executados em accept_connections().
     */
    bool configure_tcp_socket();

    /**
     * @brief Coloca o socket TCP em escuta e cria uma thread para cada cliente aceito.
     * @param args Parâmetro da thread, não utilizado.
     * @return NULL se listen() falhar ou após criar CONNECTION_LIMIT atendimentos.
     * @note Cada thread recebe um SocketParam_t alocado dinamicamente.
     * @note O limite conta atendimentos criados durante esta execução;
     *       desconexões não fazem a rotina reutilizar posições anteriores.
     */
    void* accept_connections(void *args);

    /**
     * @brief Cria as threads de anúncio e aceitação e aguarda as threads do servidor.
     * @details Aguarda primeiro as threads de conexão e depois as de atendimento.
     * @return true após concluir as chamadas de pthread_join().
     * @note A implementação atual não verifica os retornos de pthread_create()
     *       e pthread_join().
     */
    bool initConnectionThreads();

    /**
     * @brief Recebe dados do socket TCP de um cliente até o encerramento ou erro da conexão.
     * @param args Ponteiro para um SocketParam_t alocado com malloc().
     * @return NULL após atualizar o estado do cliente e fechar seu socket.
     * @details Copia o descritor recebido e libera a estrutura de argumento.
     *          Ao terminar, marca a posição correspondente em socketCients com -1,
     *          decrementa connectedClients e fecha o socket.
     * @note Atualmente imprime os dados recebidos para depuração.
     * @note Cada recv() retorna um trecho do fluxo TCP, não necessariamente
     *       uma mensagem completa.
     */
    void* handleClients(void *args);

}