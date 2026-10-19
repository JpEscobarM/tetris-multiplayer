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


namespace client
{

    inline constexpr unsigned short BROADCAST_PORT = 5001;
    inline int socketUDP = -1; /// < usado para receber a comunicacao UDP
    inline int socketTCP =-1; ///< usado pra conectar via TCP no servidor

    inline sockaddr_in enderecoClienteBroadcast;
    inline sockaddr_in enderecoServer; /// < usada para guardar o endereço do servidor ao receber o anuncio

    inline pthread_t connectionThread;

    /**
    * @brief Inicializa o cliente
    * @return true em caso de sucesso; false caso contrário.
    */
    bool init_client();


    /**
   * @brief Cria e vincula o socket UDP à porta definida em BROADCAST_PORT
   * @return true em caso de sucesso; false caso contrário.
   */
    bool configure_udp_socket();

    /**
     * @brief Aguarda o anuncio do serviidor com o IP e endereco TCP para conexao
     */
    void* listen_server_announcement(void *args);

    /**
     * @brief Inicia thread que aguarda anuncio UDP do servidor
     */
    void initConnectionThreads();


    /**
     *@brief Faz uma conexão na porta tcp indicada
     *@param  port porta TCP do servidor
     * @return true - conectou false - nao conseguiu conectar
     */
    bool connectTCP(unsigned short port );

    /**
     * @brief Extrai a porta na mensage
     * @param message
     * @return unsigned short do numero da porta  ou 0 - erro
     */
    unsigned short extract_port(char *message);

}