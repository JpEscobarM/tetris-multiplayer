#pragma once
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstdio>
#include <string>
#include <strings.h>
#include <unistd.h>
#include <iostream>
#include <pthread.h>


namespace client
{

    inline constexpr unsigned short BROADCAST_PORT = 5001;
    inline int socketUDP = -1; /// < usado para receber a comunicacao UDP

    inline sockaddr_in enderecoCliente;
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


}