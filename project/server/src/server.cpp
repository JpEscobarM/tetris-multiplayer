#include "server.hpp"


namespace server {




    bool configure_tcp_socket() {

        int ret= -1;

        socketTCP = socket(AF_INET,SOCK_STREAM,0);

        bzero((char*)&enderecoServidor,sizeof(enderecoServidor));

        enderecoServidor.sin_family = AF_INET; //ipv4
        enderecoServidor.sin_port = htons(TCP_PORT); //porta
        enderecoServidor.sin_addr.s_addr= INADDR_ANY; //aceita conexao de qualquer origem


        ret = bind(socketTCP,(struct sockaddr*)&enderecoServidor,sizeof(enderecoServidor));

        if (ret == -1) {
            perror("ERRO AO REALIZAR BIND NA PORTA 5002");
            return false;
        }


        return true;
    }



    bool configure_udp_socket() {

        int habilitarBroadcast = 1;

        //CONFIGURACAO INICIAL DO SOCKET UDP
        socketUDP = socket(AF_INET, SOCK_DGRAM, 0);

        if (socketUDP == -1) {
            perror("ERRO AO INICIAR SOCKET UDP");
            return false;
        }

        //HABILITANDO PARA BROADCAST

        setsockopt(socketUDP,SOL_SOCKET,SO_BROADCAST,&habilitarBroadcast,sizeof(habilitarBroadcast));

        bzero((char*)&enderecoBroadcast,sizeof(enderecoBroadcast));
        enderecoBroadcast.sin_family = AF_INET;
        enderecoBroadcast.sin_addr.s_addr = htonl(INADDR_BROADCAST);// 255.255.255.255
        enderecoBroadcast.sin_port = htons(BROADCAST_PORT); // porta 5001
        //=========


        return true;
    }

    bool init_server() {

        bool udpCnfigs = configure_udp_socket();
        bool tcpConfigs = configure_tcp_socket();


        if (udpCnfigs == false || tcpConfigs == false) {

            return false;
        }

            std::cout << "\nServer started";
            std::cout << "\nTCP and UDP sockets configured";

            initConnectionThreads();

        return true;
    }

   void* connection_announcement(void *args) {

        std::string message = "TETRIS|" + std::to_string(server::TCP_PORT);

        int ret=0;

        int check;

        while ( check < CONNECTION_LIMIT) {

            pthread_mutex_lock(&lock);
            check = connectedClients;
            pthread_mutex_unlock(&lock);

            printf("\n\nWaiting for connection...");
            printf("\nClients connected: %d",check);

            ret = sendto(socketUDP,message.data(),message.size(),0,(sockaddr*)&enderecoBroadcast,sizeof(enderecoBroadcast));

            if (ret == -1) {
                perror("\nERRO AO ENVIAR ANUNCIO EM BROADCAST");
            }

            sleep(TIME_TO_CHECK);
        }
        pthread_exit(NULL);
    }

    void* accept_connections(void *args) {
        int count = 0;

        if (listen(socketTCP, CONNECTION_LIMIT) == -1) {
            perror("ERRO AO ESCUTAR CONEXOES TCP");
            return NULL;
        }

        while (count < CONNECTION_LIMIT) {
            int clientSocket = accept(socketTCP, NULL, NULL);

            if (clientSocket == -1) {
                perror("ERRO AO ACEITAR CONEXAO");
                continue;
            }

            socketCients[count] = clientSocket;
            count++;

            pthread_mutex_lock(&lock);
            connectedClients++;
            pthread_mutex_unlock(&lock);
        }

        return NULL;
    }

    bool initConnectionThreads() {


        pthread_create(&connectionThreads[0],NULL,connection_announcement,NULL);// primeira thread responsavel por anunciar enquanto existirem conexoes vazias
        pthread_create(&connectionThreads[1],NULL,accept_connections,NULL);

        pthread_join(connectionThreads[0], NULL);
        pthread_join(connectionThreads[1], NULL);
        return true;
    }

    void* handleClients(void *args) {
        //implementar
    }

}
