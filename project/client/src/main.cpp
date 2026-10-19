
#include "client.hpp"

int main()
{

    client::init_client();

    //inicializa threads produtoras e consumidoras


    client::init_event_pool();

    return 0;
}