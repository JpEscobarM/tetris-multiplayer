
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


    void* producer_client_event_pool(void *args)
    {
        Event event;

        while (1)
        {
            printf("\nDigite a mensagem para o servidor: \n");

            if (fgets(event.buffer,sizeof(event.buffer),stdin) == NULL)
            {
                perror("\nERRO AO LER ENTRADA DE DADOS NO PRODUCER\n");
                pthread_exit(NULL);
            }

            event.buffer[strcspn(event.buffer,"\n")] = '\0';

            pthread_mutex_lock(&event_lock);

            while (eventPoolSize == EVENT_POOL_BUFFER_SIZE)
            {
                pthread_cond_wait(&hasSpace,&event_lock);
            }

            eventPool[writeIndex]=event;
            writeIndex = (writeIndex +1) % EVENT_POOL_BUFFER_SIZE; //circular
            eventPoolSize++;

            pthread_mutex_unlock(&event_lock);

            pthread_cond_signal(&hasEvent);


        }

        pthread_exit(NULL);
    }

    void* consumer_client_event_pool(void *args)
    {

        while (1)
        {

            pthread_mutex_lock(&event_lock);

            while (eventPoolSize == 0)
            {
                pthread_cond_wait(&hasEvent,&event_lock);

            }

            Event event = eventPool[readIndex];
            readIndex = (readIndex+1) %EVENT_POOL_BUFFER_SIZE;
            eventPoolSize--;

            pthread_mutex_unlock(&event_lock);
            pthread_cond_signal(&hasSpace);

            if (socketTCP == -1)
            {
                perror("\nSOCKET INDISPONIVEL, NAO É POSSIVEL ENVIAR DADOS AO SERVIDOR\n");
                continue;
            }

            size_t total = strlen(event.buffer) + 1;
            size_t sent = 0;

            while (sent < total) {
                ssize_t result = send(
                    socketTCP,
                    event.buffer + sent,
                    total - sent,
                    MSG_NOSIGNAL
                );

                if (result == -1) {
                    if (errno == EINTR) continue;

                    perror("ERRO AO ENVIAR EVENTO");
                    return NULL;
                }

                if (result == 0) {
                    fprintf(stderr, "ENVIO SEM PROGRESSO\n");
                    return NULL;
                }

                sent += (size_t)result;
            }

        }
        pthread_exit(NULL);
    }
    void init_event_pool() {
        int error;

        error = pthread_create(&eventConsumerThread, NULL, consumer_client_event_pool, NULL);
        if (error != 0) {
            fprintf(stderr, "ERRO AO CRIAR CONSUMIDOR: %s\n", strerror(error));
            exit(EXIT_FAILURE);
        }

        error = pthread_create(&eventProducerThread, NULL, producer_client_event_pool, NULL);
        if (error != 0) {
            fprintf(stderr, "ERRO AO CRIAR PRODUTOR: %s\n", strerror(error));
            exit(EXIT_FAILURE);
        }

        pthread_join(eventProducerThread, NULL);
        pthread_join(eventConsumerThread, NULL);
    }

}