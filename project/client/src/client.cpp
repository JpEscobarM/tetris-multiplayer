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

        // Permite abrir dois clientes na mesma maquina escutando a mesma porta.
        setsockopt(socketUDP, SOL_SOCKET, SO_REUSEADDR, &reutilizarEndereco, sizeof(reutilizarEndereco));

        // Timeout de 1s no recvfrom: assim a thread consegue perceber que o jogo foi fechado.
        timeval tv{};
        tv.tv_sec = 1;
        setsockopt(socketUDP, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        memset(&enderecoClienteBroadcast, 0, sizeof(enderecoClienteBroadcast));
        enderecoClienteBroadcast.sin_family = AF_INET;
        enderecoClienteBroadcast.sin_addr.s_addr = htonl(INADDR_ANY);
        enderecoClienteBroadcast.sin_port = htons(BROADCAST_PORT);

        if (bind(socketUDP, (struct sockaddr *)&enderecoClienteBroadcast, sizeof(enderecoClienteBroadcast)) == -1) {
            perror("ERRO AO REALIZAR BIND UDP");
            close(socketUDP);
            socketUDP = -1;
            return false;
        }

        return true;
    }

    unsigned short extract_port(char *message) {
        unsigned long port = 0;

        if (message == NULL) return 0;

        char *st1 = strtok(message, "|");
        char *st2 = strtok(NULL, "|");

        if (st1 == NULL || st2 == NULL || strstr(st1, "TETRIS") == NULL) {
            return 0;   // nao e um anuncio do nosso servidor
        }

        for (char *p = st2; *p != '\0'; ++p) {
            if (*p < '0' || *p > '9') return 0;

            port = port * 10 + (*p - '0');

            if (port > 65535) return 0;
        }

        return (unsigned short)port;
    }

    bool connectTCP(unsigned short port) {

        if (port == 0) {
            fprintf(stderr, "PORTA TCP INVALIDA\n");
            return false;
        }

        socketTCP = socket(AF_INET, SOCK_STREAM, 0);

        if (socketTCP == -1) {
            perror("ERRO AO CRIAR SOCKET TCP");
            return false;
        }

        enderecoServer.sin_family = AF_INET;
        enderecoServer.sin_port = htons(port);

        if (connect(socketTCP, (struct sockaddr *)&enderecoServer, sizeof(enderecoServer)) == -1) {
            perror("ERRO AO CONECTAR SOCKET TCP");
            close(socketTCP);
            socketTCP = -1;
            return false;
        }

        // Sem atraso do algoritmo de Nagle: queremos que cada movimento chegue logo.
        int nodelay = 1;
        setsockopt(socketTCP, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay));

        return true;
    }

    // Dorme em pedacinhos para poder acordar logo se o jogo for fechado.
    static void sleep_interruptible(int ms) {
        for (int i = 0; i < ms / 50 && !stopping; i++) usleep(50 * 1000);
    }

    void *connection_routine(void *args) {
        (void)args;
        bool ok = false;

        if (manualServerIp[0] != '\0') {
            // Modo manual: ./tetris_client 192.168.0.10
            memset(&enderecoServer, 0, sizeof(enderecoServer));
            if (inet_pton(AF_INET, manualServerIp, &enderecoServer.sin_addr) != 1) {
                fprintf(stderr, "IP INVALIDO: %s\n", manualServerIp);
                connectionLost = true;
                return NULL;
            }
            while (!stopping && !ok) {
                ok = connectTCP(TCP_PORT);
                if (!ok) sleep_interruptible(1000);
            }
        } else {
            if (!configure_udp_socket()) {
                connectionLost = true;
                return NULL;
            }

            printf("[cliente] procurando servidor (broadcast UDP porta %d)...\n", BROADCAST_PORT);

            while (!stopping && !ok) {
                char message[1024];
                socklen_t addressSize = sizeof(enderecoServer);

                ssize_t n = recvfrom(socketUDP, message, sizeof(message) - 1, 0,
                                     (struct sockaddr *)&enderecoServer, &addressSize);
                if (n < 0) continue;            // timeout (ou sinal): tenta de novo

                message[n] = '\0';
                unsigned short port = extract_port(message);
                if (port == 0) continue;

                ok = connectTCP(port);
                if (!ok) sleep_interruptible(1000);
            }
            close(socketUDP);
            socketUDP = -1;
        }

        if (!ok) return NULL;   // fechou o jogo antes de conectar

        printf("[cliente] conectado ao servidor\n");

        int error = pthread_create(&eventConsumerThread, NULL, consumer_client_event_pool, NULL);
        if (error != 0) {
            fprintf(stderr, "ERRO AO CRIAR THREAD DE ENVIO: %s\n", strerror(error));
            connectionLost = true;
            return NULL;
        }
        error = pthread_create(&receiverThread, NULL, receiver_routine, NULL);
        if (error != 0) {
            fprintf(stderr, "ERRO AO CRIAR THREAD DE RECEPCAO: %s\n", strerror(error));
            connectionLost = true;
            return NULL;
        }
        ioThreadsStarted = true;
        connected = true;
        return NULL;
    }

    void start_connection(const char *serverIp) {
        manualServerIp[0] = '\0';
        if (serverIp != nullptr) {
            strncpy(manualServerIp, serverIp, sizeof(manualServerIp) - 1);
            manualServerIp[sizeof(manualServerIp) - 1] = '\0';
        }
        stopping = false;
        connected = false;
        connectionLost = false;

        int error = pthread_create(&connectionThread, NULL, connection_routine, NULL);
        if (error != 0) {
            fprintf(stderr, "ERRO AO CRIAR THREAD DE CONEXAO: %s\n", strerror(error));
            connectionLost = true;
            return;
        }
        connectionThreadStarted = true;
    }

    bool is_connected() { return connected; }
    bool connection_lost() { return connectionLost; }

    // PRODUTOR: o jogo chama esta funcao; ela so poe a mensagem na fila.
    bool send_message(MessageType type, const void *data, size_t len) {
        if (!connected || len > PAYLOAD_SIZE) return false;

        Message m;
        memset(&m, 0, sizeof(m));
        m.option = type;
        if (data != nullptr && len > 0) memcpy(m.payload, data, len);

        pthread_mutex_lock(&event_lock);
        if (eventPoolSize == EVENT_POOL_BUFFER_SIZE) {
            pthread_mutex_unlock(&event_lock);
            return false;
        }
        eventPool[writeIndex] = m;
        writeIndex = (writeIndex + 1) % EVENT_POOL_BUFFER_SIZE;
        eventPoolSize++;
        pthread_mutex_unlock(&event_lock);
        pthread_cond_signal(&hasEvent);
        return true;
    }

    // CONSUMIDOR: retira da fila em ordem (FIFO) e envia pelo TCP.
    void *consumer_client_event_pool(void *args)
    {
        (void)args;
        while (true)
        {
            pthread_mutex_lock(&event_lock);

            while (eventPoolSize == 0 && !stopping) {
                pthread_cond_wait(&hasEvent, &event_lock);
            }

            if (eventPoolSize == 0) {            // stopping e a fila ja esvaziou
                pthread_mutex_unlock(&event_lock);
                return NULL;
            }

            Message event = eventPool[readIndex];
            readIndex = (readIndex + 1) % EVENT_POOL_BUFFER_SIZE;
            eventPoolSize--;

            pthread_mutex_unlock(&event_lock);

            const char *data = (const char *)&event;
            size_t total = sizeof(event);
            size_t sent = 0;

            while (sent < total)
            {
                ssize_t result = send(socketTCP, data + sent, total - sent, MSG_NOSIGNAL);

                if (result == -1) {
                    if (errno == EINTR) continue;
                    if (!stopping) perror("ERRO AO ENVIAR MENSAGEM");
                    connectionLost = true;
                    return NULL;
                }
                if (result == 0) {
                    connectionLost = true;
                    return NULL;
                }
                sent += static_cast<size_t>(result);
            }
        }
    }

    // RECEPCAO: junta bytes ate completar uma Message inteira (TCP e um fluxo de bytes).
    void *receiver_routine(void *args)
    {
        (void)args;
        Message m;
        char *data = (char *)&m;
        size_t got = 0;

        while (!stopping)
        {
            ssize_t r = recv(socketTCP, data + got, sizeof(m) - got, 0);

            if (r > 0) {
                got += (size_t)r;
                if (got < sizeof(m)) continue;
                got = 0;

                m.payload[PAYLOAD_SIZE - 1] = '\0';
                pthread_mutex_lock(&inbox_lock);
                if (inCount < INBOX_SIZE) {
                    inbox[(inHead + inCount) % INBOX_SIZE] = m;
                    inCount++;
                }
                pthread_mutex_unlock(&inbox_lock);
            } else if (r == 0) {
                if (!stopping) connectionLost = true;
                return NULL;
            } else {
                if (errno == EINTR) continue;
                if (!stopping) connectionLost = true;
                return NULL;
            }
        }
        return NULL;
    }

    bool poll_message(Message &out) {
        bool has = false;
        pthread_mutex_lock(&inbox_lock);
        if (inCount > 0) {
            out = inbox[inHead];
            inHead = (inHead + 1) % INBOX_SIZE;
            inCount--;
            has = true;
        }
        pthread_mutex_unlock(&inbox_lock);
        return has;
    }

    void shutdown_client() {
        // 1) espera a thread de conexao terminar (ela acorda sozinha por causa dos timeouts)
        if (connected) send_message(MSG_DISCONNECT, nullptr, 0);   // avisa o servidor
        stopping = true;
        pthread_cond_broadcast(&hasEvent);

        if (connectionThreadStarted) {
            pthread_join(connectionThread, NULL);
            connectionThreadStarted = false;
        }

        if (ioThreadsStarted) {
            pthread_join(eventConsumerThread, NULL);        // esvazia a fila (inclui o DISCONNECT)
            if (socketTCP != -1) shutdown(socketTCP, SHUT_RDWR);   // acorda o recv()
            pthread_join(receiverThread, NULL);
            ioThreadsStarted = false;
        }

        if (socketTCP != -1) { close(socketTCP); socketTCP = -1; }
        if (socketUDP != -1) { close(socketUDP); socketUDP = -1; }
        connected = false;
    }

}
