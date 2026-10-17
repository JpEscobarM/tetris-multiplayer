
#include "client.hpp"

namespace client
{

    bool configure_udp_socket() {

        int reutilizarEndereco = 1;

        //CONFIGURACAO INICIAL DO SOCKET UDP
        socketUDP = socket(AF_INET, SOCK_DGRAM, 0);

        if (socketUDP == -1) {
            perror("ERRO AO INICIAR SOCKET UDP");
            return false;
        }

        setsockopt(socketUDP,SOL_SOCKET,SO_REUSEADDR,&reutilizarEndereco,sizeof(reutilizarEndereco));

        bzero((char*)&enderecoCliente,sizeof(enderecoCliente));
        enderecoCliente.sin_family = AF_INET;
        enderecoCliente.sin_addr.s_addr = htonl(INADDR_ANY);
        enderecoCliente.sin_port = htons(BROADCAST_PORT);

        int bindRet;
        bindRet = bind(socketUDP, (struct sockaddr*)&enderecoCliente, sizeof(enderecoCliente));

    if (bindRet == -1) {
        close(socketUDP);
        perror("ERRO AO REALIZAR BIND UDP");
        return false;
    }

        return true;
    }

    void* listen_server_announcement(void *args) {

       int recvRet;
        char message[1024];

        socklen_t addressSize = sizeof(enderecoServer);

        while (1) {

            recvRet= recvfrom(socketUDP,message,1024,0,(struct sockaddr*)&enderecoServer,&addressSize);

            if (recvRet != -1) break;

        }

        message[recvRet] = '\0';

        printf("Received message from server: %s\n", message);

        pthread_exit(NULL);
    }


    void initConnectionThreads() {

        pthread_create(&connectionThread,NULL,listen_server_announcement,NULL);
        pthread_join(connectionThread,NULL);

    }


    bool init_client() {


        bool udpConfigs = configure_udp_socket();

        if (udpConfigs == false) {

            return false;
        }

        std::cout << "\n";
        std::cout << "\nUDP socket configured";

        initConnectionThreads();

        return true;
    }

}