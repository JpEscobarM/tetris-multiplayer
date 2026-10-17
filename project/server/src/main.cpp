//
// Created by jmartins on 15/10/2026.
//

#include "server.hpp"
#include <iostream>


int main()
{

    if (server::init_server()) {
        std::cout << "\nAll clients connected";
    }



    return 0;
}
