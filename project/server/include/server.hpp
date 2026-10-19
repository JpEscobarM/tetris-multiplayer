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


    typedef struct
    {
        int socket;

    }SocketParam_t; /// < Tipo usado para passar um socket como parametro para uma funcao handleClient responsavel por receber as informacoes do cliente


    inline constexpr unsigned short BROADCAST_PORT = 5001;
    inline constexpr unsigned short TCP_PORT = 5002;
    inline constexpr unsigned short TIME_TO_CHECK = 3;

    inline int CONNECTION_LIMIT = 2; /// < limite de conexoes simultaneas

    inline int socketCients[2];
    inline int connectedClients = 0; ///< Quantidade de clientes conectados.

    inline   int socketUDP = -1;      ///< Socket UDP de descoberta.
    inline int socketTCP = -1; ///<Socket TCP utilizado para receber conexoes TCP


    inline  sockaddr_in enderecoServidor; ///< usado para aceitar conexoes TCP
    inline sockaddr_in enderecoBroadcast; ///< usado para fazer o anuncio do IP BROADCAST

   inline pthread_t connectionThreads[2]; /// < threads responsáveis por fazer o anuncio UDP e aceitar conexoes TCP
   inline pthread_t handleClientsThreads[2]; /// < recebem as informações do socket


    inline pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

    /**
     * @brief Inicializa o servidor.
     * @return true em caso de sucesso; false caso contrário.
     */
    bool init_server();

    /**
     * @brief Cria e vincula o socket UDP à porta definida em BROADCAST_PORT
     * @return true em caso de sucesso; false caso contrário.
     */
    bool configure_udp_socket();


    /**@brief Envia mensagens aos clientes avisando que está pronto para receber conexoes
     * @param timeToCheck variavel que define o tempo entre uma mensagem e outra
     */
    void* connection_announcement(void *args);

    /**@brief Inicia socket que aceita conexões TCP dos clientes
     * @return true em caso de sucesso; false caso contrário.
     */
    bool configure_tcp_socket();

    /**@brief Aceita conexoes TCP dos clientes
     */
    void* accept_connections(void *args);

    /**@brief Inicia thread de envio de conexao na rede e thread que aceita conexao dos clientes
     */
    bool initConnectionThreads();

    /**
     * @brief Função que recebe os dados a partir do socketTCP passado em args
     *
     * @param args parametro com o socket a ser verificado
     * @return pthread_exit
     */
    void* handleClients(void *args);

}