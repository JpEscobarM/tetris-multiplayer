
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

        bzero((char*)&enderecoClienteBroadcast,sizeof(enderecoClienteBroadcast));
        enderecoClienteBroadcast.sin_family = AF_INET;
        enderecoClienteBroadcast.sin_addr.s_addr = htonl(INADDR_ANY);
        enderecoClienteBroadcast.sin_port = htons(BROADCAST_PORT);

        int bindRet;
        bindRet = bind(socketUDP, (struct sockaddr*)&enderecoClienteBroadcast, sizeof(enderecoClienteBroadcast));

    if (bindRet == -1) {
        close(socketUDP);
        perror("ERRO AO REALIZAR BIND UDP");
        return false;
    }

        return true;
    }


    unsigned short extract_port(char *message) {
        unsigned long port = 0;

        if (message == NULL) return 0;

        char *st1 = strtok(message, "|");
        char *st2 = strtok(NULL, "|");

        if (st1 == NULL || st2 == NULL) {
            fprintf(stderr, "ERRO AO EXTRAIR PORTA\n");
            return 0;
        }

        for (char *p = st2; *p != '\0'; ++p) {
            if (*p < '0' || *p > '9') return 0;

            port = port * 10 + (*p - '0');

            if (port > 65535) return 0;
        }

        return (unsigned short)port;
    }

    bool connectTCP(unsigned short port ) {

        if (port == 0 ) {
            perror("PORTA COM ENDERECO TCP = 0");
            return false;
        }

        socketTCP= socket(AF_INET, SOCK_STREAM, 0);

        if (socketTCP  == -1) {
            perror("ERRO AO CRIAR SOCKET TCP");
            return false;
        }

        enderecoServer.sin_family = AF_INET;
        enderecoServer.sin_port = htons(port);

        if (connect(socketTCP,(struct sockaddr*)&enderecoServer,sizeof(enderecoServer)) == -1) {

            perror("ERRO AO CONECTAR SOCKET TCP");
            close(socketTCP);
            socketTCP = -1;
            return false;
        }

    return true;
    }



    void* listen_server_announcement(void *args) {

       int recvRet;
        char message[1024];
        unsigned short port;

        socklen_t addressSize = sizeof(enderecoServer);

        while (1) {

            recvRet= recvfrom(socketUDP,message,1024,0,(struct sockaddr*)&enderecoServer,&addressSize);

            if (recvRet != -1) break;

        }

        message[recvRet] = '\0';

        port = extract_port(message);



        bool connected = connectTCP(port);

        if (!connected) {
            perror("ERRO AO CONECTAR TCP");
        }

        pthread_exit(NULL);
    }


    void initConnectionThreads() {


        printf("\nIniciando escuta");

        pthread_create(&connectionThread,NULL,listen_server_announcement,NULL);

        printf("\nUDP recebido");

        pthread_join(connectionThread,NULL);

    }


    bool init_client() {


        bool udpConfigs = configure_udp_socket();

        if (udpConfigs == false) {

            return false;
        }

        std::cout << "\n";
        std::cout << "\nUDP socket configured\n";

        initConnectionThreads();

        return true;
    }

}