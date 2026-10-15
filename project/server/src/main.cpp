//
// Created by jmartins on 15/10/2026.
//

#include "server.hpp"
#include <iostream>


int main()
{
    std::string  frase= "Esse print vem do servidor";
    server::print(frase);

    return 0;
}
