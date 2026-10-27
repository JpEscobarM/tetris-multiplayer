#include "server.hpp"

namespace server {

    bool configure_tcp_socket() {

        socketTCP = socket(AF_INET, SOCK_STREAM, 0);

        if (socketTCP == -1) {
            perror("ERRO AO CRIAR SOCKET TCP");
            return false;
        }

        // Permite reiniciar o servidor logo apos encerrar (evita "Address already in use").
        int reutilizarEndereco = 1;
        setsockopt(socketTCP, SOL_SOCKET, SO_REUSEADDR, &reutilizarEndereco, sizeof(reutilizarEndereco));

        memset(&enderecoServidor, 0, sizeof(enderecoServidor));
        enderecoServidor.sin_family = AF_INET;                 // ipv4
        enderecoServidor.sin_port = htons(TCP_PORT);           // porta
        enderecoServidor.sin_addr.s_addr = htonl(INADDR_ANY);  // aceita conexao de qualquer origem

        if (bind(socketTCP, (struct sockaddr*)&enderecoServidor, sizeof(enderecoServidor)) == -1) {
            perror("ERRO AO REALIZAR BIND NA PORTA TCP");
            return false;
        }

        return true;
    }

    bool configure_udp_socket() {

        int habilitarBroadcast = 1;

        socketUDP = socket(AF_INET, SOCK_DGRAM, 0);

        if (socketUDP == -1) {
            perror("ERRO AO INICIAR SOCKET UDP");
            return false;
        }

        setsockopt(socketUDP, SOL_SOCKET, SO_BROADCAST, &habilitarBroadcast, sizeof(habilitarBroadcast));

        memset(&enderecoBroadcast, 0, sizeof(enderecoBroadcast));
        enderecoBroadcast.sin_family = AF_INET;
        enderecoBroadcast.sin_addr.s_addr = htonl(INADDR_BROADCAST);   // 255.255.255.255
        enderecoBroadcast.sin_port = htons(BROADCAST_PORT);            // porta 5001

        return true;
    }

    bool init_server() {

        if (!configure_udp_socket() || !configure_tcp_socket()) {
            return false;
        }

        printf("\n--- Servidor iniciado ---\n");
        printf("Sockets TCP (porta %d) e UDP (broadcast na porta %d) configurados\n", TCP_PORT, BROADCAST_PORT);

        return initConnectionThreads();
    }

    // Anuncia o servidor em broadcast enquanto ainda houver vaga (ex.: jogador 2 ainda nao chegou).
    void* connection_announcement(void *args) {
        (void)args;

        std::string message = "- TETRIS|" + std::to_string(TCP_PORT);
        int ultimo = -1;

        while (true) {
            pthread_mutex_lock(&lock);
            int conectados = connectedClients;
            pthread_mutex_unlock(&lock);

            if (conectados != ultimo) {      // so imprime quando muda (evita poluir o console)
                printf("Jogadores conectados: %d/%d\n", conectados, CONNECTION_LIMIT);
                ultimo = conectados;
            }

            if (conectados < CONNECTION_LIMIT) {
                ssize_t ret = sendto(socketUDP, message.data(), message.size(), 0,
                                     (sockaddr*)&enderecoBroadcast, sizeof(enderecoBroadcast));
                if (ret == -1) {
                    perror("ERRO AO ENVIAR ANUNCIO EM BROADCAST");
                }
            }

            sleep(TIME_TO_CHECK);
        }
        return NULL;
    }

    void* accept_connections(void *args) {
        (void)args;

        if (listen(socketTCP, CONNECTION_LIMIT) == -1) {
            perror("ERRO AO ESCUTAR CONEXOES TCP");
            return NULL;
        }

        while (true) {
            int clientSocket = accept(socketTCP, NULL, NULL);

            if (clientSocket == -1) {
                if (errno != EINTR && errno != ECONNABORTED) {
                    perror("ERRO AO ACEITAR CONEXAO");
                    sleep(1);   // evita girar em falso se o erro persistir
                }
                continue;
            }

            int nodelay = 1;
            setsockopt(clientSocket, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay));

            // Procura uma vaga livre (0 ou 1).
            int slot = -1;
            pthread_mutex_lock(&lock);
            for (int i = 0; i < CONNECTION_LIMIT; i++) {
                if (socketClients[i] == -1) { slot = i; break; }
            }
            if (slot != -1) {
                socketClients[slot] = clientSocket;
                joined[slot] = false;
                connectedClients++;
            }
            pthread_mutex_unlock(&lock);

            if (slot == -1) {
                printf("Conexao recusada: servidor cheio\n");
                close(clientSocket);
                continue;
            }

            SocketParam_t *param = (SocketParam_t *)malloc(sizeof(SocketParam_t));
            pthread_t tid;
            int err_t = (param == NULL) ? ENOMEM : 0;

            if (param != NULL) {
                param->socket = clientSocket;
                param->id = slot;
                err_t = pthread_create(&tid, NULL, handleClients, param);
            }

            if (err_t != 0) {
                fprintf(stderr, "ERRO AO CRIAR THREAD: %s\n", strerror(err_t));
                free(param);
                pthread_mutex_lock(&lock);
                socketClients[slot] = -1;
                connectedClients--;
                pthread_mutex_unlock(&lock);
                close(clientSocket);
                continue;
            }

            pthread_detach(tid);   // a thread se limpa sozinha quando o jogador sai
            printf("Jogador %d conectou\n", slot);
        }
        return NULL;
    }

    bool initConnectionThreads() {

        pthread_create(&connectionThreads[0], NULL, connection_announcement, NULL);
        pthread_create(&connectionThreads[1], NULL, accept_connections, NULL);

        // O servidor fica de pe para sempre (varias partidas, jogadores entrando e saindo).
        pthread_join(connectionThreads[0], NULL);
        pthread_join(connectionThreads[1], NULL);

        return true;
    }

    // Envia uma Message para o jogador 'id'. EXIGE que 'lock' ja esteja travado:
    // assim o socket nao pode ser fechado por outra thread no meio do envio,
    // e dois envios para o mesmo socket nunca se misturam.
    static void send_to_locked(int id, MessageType type, const void *data, size_t len) {
        int sock = socketClients[id];
        if (sock == -1) return;               // jogador ja saiu

        Message m;
        memset(&m, 0, sizeof(m));
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

    // Termina a partida e avisa os DOIS jogadores ao mesmo tempo. Exige 'lock' travado.
    static void end_game_locked(int winnerId) {
        gameRunning = false;
        joined[0] = joined[1] = false;      // para jogar de novo, cada um manda MSG_JOIN outra vez
        EndPayload ep;
        ep.winnerId = winnerId;
        for (int i = 0; i < CONNECTION_LIMIT; i++) {
            send_to_locked(i, MSG_GAME_END, &ep, sizeof(ep));
        }
        printf("FIM DE JOGO: jogador %d venceu\n", winnerId);
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

                // Garante terminador mesmo se o cliente mandar 256 bytes "cheios".
                message.payload[PAYLOAD_SIZE - 1] = '\0';

                if (message.option == MSG_DISCONNECT) {
                    printf("Jogador %d pediu para desconectar\n", myId);
                    break;
                }

                switch (message.option) {
                    case MSG_CHAT:
                        printf("MENSAGEM DO JOGADOR %d: %s\n", myId, message.payload);
                        break;

                    case MSG_JOIN: {
                        pthread_mutex_lock(&lock);
                        if (!gameRunning) {
                            joined[myId] = true;
                            printf("Jogador %d esta pronto\n", myId);
                            if (joined[0] && joined[1]) {
                                // (1) a semente: os dois jogadores recebem a MESMA, entao as pecas sao iguais
                                int32_t seed = (int32_t)(rng() & 0x7fffffff);
                                gameRunning = true;
                                for (int i = 0; i < CONNECTION_LIMIT; i++) {
                                    StartPayload sp;
                                    sp.seed = seed;
                                    sp.playerId = i;
                                    send_to_locked(i, MSG_START, &sp, sizeof(sp));
                                }
                                printf("PARTIDA INICIADA: seed=%d\n", seed);
                            }
                        }
                        pthread_mutex_unlock(&lock);
                        break;
                    }

                    case MSG_ATTACK: {
                        // (2) intermedia o ataque: valida, sorteia o buraco e entrega ao adversario
                        AttackPayload ap;
                        memcpy(&ap, message.payload, sizeof(ap));
                        if (ap.lines < 1 || ap.lines > 4) {
                            fprintf(stderr, "Ataque invalido ignorado (jogador %d: %d linhas)\n", myId, ap.lines);
                            break;
                        }
                        pthread_mutex_lock(&lock);
                        if (gameRunning) {
                            ap.holeCol = (int32_t)(rng() % BOARD_COLS);
                            send_to_locked(otherId, MSG_ATTACK, &ap, sizeof(ap));
                            printf("ATAQUE: jogador %d -> jogador %d, %d linha(s), buraco na coluna %d\n",
                                   myId, otherId, ap.lines, ap.holeCol);
                        }
                        pthread_mutex_unlock(&lock);
                        break;
                    }

                    case MSG_GAME_STATE:
                        // So repassa a "foto" do tabuleiro para o adversario desenhar.
                        pthread_mutex_lock(&lock);
                        if (gameRunning)
                            send_to_locked(otherId, MSG_GAME_STATE, message.payload, PAYLOAD_SIZE);
                        pthread_mutex_unlock(&lock);
                        break;

                    case MSG_GAME_OVER:
                        // (3) quem perdeu avisa; o servidor encerra para os dois ao mesmo tempo
                        pthread_mutex_lock(&lock);
                        if (gameRunning) end_game_locked(otherId);
                        pthread_mutex_unlock(&lock);
                        break;

                    default:
                        fprintf(stderr, "TIPO DE MENSAGEM DESCONHECIDO: %d\n", (int)message.option);
                        break;
                }

                receivedTotal = 0;
                memset(&message, 0, sizeof(message));

            } else if (received == 0) {
                if (receivedTotal != 0) {
                    fprintf(stderr, "CONEXAO ENCERRADA COM MENSAGEM INCOMPLETA (%zu/%zu bytes)\n",
                            receivedTotal, sizeof(message));
                }
                printf("Jogador %d encerrou a conexao\n", myId);
                break;
            } else {
                if (errno == EINTR) {
                    continue; // Interrompido por um sinal; tenta novamente.
                }

                perror("ERRO AO RECEBER DADOS DO CLIENTE");
                break;
            }
        }

        pthread_mutex_lock(&lock);

        // Se o jogador saiu NO MEIO da partida, o adversario vence.
        if (gameRunning) {
            end_game_locked(otherId);
        }

        joined[myId] = false;
        socketClients[myId] = -1;
        connectedClients--;

        pthread_mutex_unlock(&lock);

        close(clientSocket);
        return NULL;
    }

}
