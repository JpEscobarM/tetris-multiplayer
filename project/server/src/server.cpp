#include "server.hpp"
#include <cerrno>
#include <random>



namespace server {
    bool configure_tcp_socket() {

        int ret= -1;

        socketTCP = socket(AF_INET,SOCK_STREAM,0);

        if (socketTCP == -1) {
            perror("ERRO AO CRIAR SOCKET TCP");
            return false;
        }

        // Permite reiniciar o servidor logo apos encerrar (evita "Address already in use").
        int reutilizarEndereco = 1;
        setsockopt(socketTCP, SOL_SOCKET, SO_REUSEADDR, &reutilizarEndereco, sizeof(reutilizarEndereco));

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

        int check = 0;

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
            param->id = count;

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



    // Envia uma Message para o jogador 'id'. EXIGE que 'lock' ja esteja travado:
    // assim o socket nao pode ser fechado por outra thread no meio do envio,
    // e dois envios para o mesmo socket nunca se misturam.
    static void send_to_locked(int id, MessageType type, const void *data, size_t len) {
        int sock = socketCients[id];
        if (sock == -1) return;               // jogador ja saiu

        Message m{};
        m.option = type;
        if (data != nullptr && len <= PAYLOAD_SIZE) memcpy(m.payload, data, len);

        const char *p = (const char *)&m;
        size_t sent = 0;
        while (sent < sizeof(m)) {
            ssize_t r = send(sock, p + sent, sizeof(m) - sent, MSG_NOSIGNAL);
            if (r < 0 && errno == EINTR) continue;
            if (r <= 0) { perror("ERRO AO ENVIAR AO CLIENTE"); return; }
            sent += (size_t)r;
        }
    }

    

    void* handleClients(void *args) {

        SocketParam_t *param = (SocketParam_t *)args;

        int clientSocket = param->socket;
        const int myId = param->id;
        const int otherId = 1 - myId;

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

                // Garante terminador mesmo se o cliente mandar 256 bytes "cheios".
                message.payload[PAYLOAD_SIZE - 1] = '\0';

                switch (message.option) {
                    case MSG_CHAT:
                        printf("\nMENSAGEM DO CLIENTE %d: %s\n", myId, message.payload);
                        break;

                    case MSG_JOIN: {
                        pthread_mutex_lock(&lock);
                        joined[myId] = true;
                        if (joined[0] && joined[1] && !gameRunning) {
                            std::random_device rd;
                            int32_t seed = (int32_t)(rd() & 0x7fffffff);
                            gameRunning = true;
                            for (int i = 0; i < 2; i++) {
                                StartPayload sp{seed, i};
                                send_to_locked(i, MSG_START, &sp, sizeof(sp));
                            }
                            printf("\nPARTIDA INICIADA, seed=%d\n", seed);
                        }
                        pthread_mutex_unlock(&lock);
                        break;
                    }

                    case MSG_ATTACK:
                    case MSG_GAME_STATE:
                        // O servidor so repassa para o adversario.
                        pthread_mutex_lock(&lock);
                        if (gameRunning)
                            send_to_locked(otherId, message.option, message.payload, PAYLOAD_SIZE);
                        pthread_mutex_unlock(&lock);
                        break;

                    case MSG_GAME_OVER: {
                        pthread_mutex_lock(&lock);
                        if (gameRunning) {
                            gameRunning = false;
                            EndPayload ep{otherId};
                            for (int i = 0; i < 2; i++)
                                send_to_locked(i, MSG_GAME_END, &ep, sizeof(ep));
                            printf("\nFIM DE JOGO: jogador %d venceu\n", otherId);
                        }
                        pthread_mutex_unlock(&lock);
                        break;
                    }

                    default:
                        fprintf(stderr, "TIPO DE MENSAGEM DESCONHECIDO: %d\n", (int)message.option);
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