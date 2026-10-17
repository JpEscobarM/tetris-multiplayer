
#include "client.hpp"

int main()
{
    client::init_client();


    while (true) {
        sleep(5);
        printf("\ndormindo");
    }


    return 0;
}