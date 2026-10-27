#include "server.hpp"
#include <iostream>

int main()
{
    // Nao retorna enquanto o servidor estiver no ar (Ctrl+C / botao Stop para encerrar).
    if (!server::init_server()) {
        std::cerr << "\nNao foi possivel iniciar o servidor\n";
        return 1;
    }

    return 0;
}
