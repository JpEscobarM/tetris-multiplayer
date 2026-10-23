#include "server.hpp"
#include <cerrno>


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

            SocketParam_t *param =(SocketParam_t *)malloc(sizeof(SocketParam_t));

            if (param == NULL) {
                perror("ERRO AO ALOCAR PARAMETRO");
                close(clientSocket);
                continue;
            }

            param->socket = clientSocket;

            socketCients[count] = clientSocket;

            int err_t = pthread_create(&handleClientsThreads[count],NULL,handleClients,param);

            pthread_mutex_lock(&lock);

            if (err_t == 0) {
                connectedClients++;
            } else {
                socketCients[count] = -1;
            }

            pthread_mutex_unlock(&lock);

            if (err_t != 0) {
                fprintf(stderr, "ERRO AO CRIAR THREAD: %s\n",strerror(err_t));

                free(param);
                close(clientSocket);
                continue;
            }

            count++;
        }

        return NULL;
    }


    bool initConnectionThreads() {


        pthread_create(&connectionThreads[0],NULL,connection_announcement,NULL);// primeira thread responsavel por anunciar enquanto existirem conexoes vazias
        pthread_create(&connectionThreads[1],NULL,accept_connections,NULL);


        pthread_join(connectionThreads[0], NULL);
        pthread_join(connectionThreads[1], NULL);

        // Espera cada thread de atendimento terminar.
        for (int i = 0; i < CONNECTION_LIMIT; i++) {
            pthread_join(handleClientsThreads[i], NULL);
        }

        return true;
    }



    void* handleClients(void *args) {

        SocketParam_t *param = (SocketParam_t *)args;

        int clientSocket = param->socket;

        free(param);

        Message message;

        char *data = (char*) &message;

        size_t receivedTotal = 0;

        while (true) {
            ssize_t received = recv(clientSocket, data + receivedTotal,
                                    sizeof(message) - receivedTotal, 0);

            if (received > 0) {
                receivedTotal += static_cast<size_t>(received);

                // Processa somente depois de receber todos os bytes da mensagem.
                if (receivedTotal < sizeof(message)) {
                    continue;
                }

                if (message.option == MSG_DISCONNECT) {
                    printf("\nCliente solicitou desconexao\n");
                    break;
                }

                switch (message.option) {
                    case MSG_CHAT:
                        // Limita a leitura mesmo se o payload vier sem terminador.
                        printf("\nMENSAGEM DO CLIENTE: %s\n",message.payload);
                        break;
                    case MSG_JOIN:
                    case MSG_MOVE:
                    case MSG_GAME_STATE:
                        printf("\nMensagem tipo %d recebida; processamento ainda nao implementado\n",
                               static_cast<int>(message.option));
                        break;
                    default:
                        fprintf(stderr, "TIPO DE MENSAGEM DESCONHECIDO\n");
                        break;
                }

                receivedTotal = 0;
                message = Message{};

            } else if (received == 0) {
                if (receivedTotal != 0) {
                    fprintf(stderr, "CONEXAO ENCERRADA COM MENSAGEM INCOMPLETA (%zu/%zu bytes)\n",
                            receivedTotal, sizeof(message));
                }
                printf("\nCliente encerrou a conexao\n");
                break;
            } else {
                if (errno == EINTR) {
                    continue; // Interrompido por um sinal; tenta novamente.
                }

                perror("\nERRO AO RECEBER DADOS DO CLIENTE");
                break;
            }
        }

        pthread_mutex_lock(&lock);

        for (int i = 0; i < CONNECTION_LIMIT; i++) {
            if (socketCients[i] == clientSocket) {
                socketCients[i] = -1;
                break;
            }
        }

        connectedClients--;

        pthread_mutex_unlock(&lock);

        close(clientSocket);
        return NULL;
    }



}
