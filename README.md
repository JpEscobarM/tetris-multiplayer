# Tetris Multiplayer

Projeto de Tetris competitivo para dois jogadores, desenvolvido para aprender C++, interfaces gráficas, sockets e programação concorrente.

O jogo está **funcional**: o servidor anuncia-se por UDP, dois clientes conectam por TCP, recebem a mesma semente (mesma sequência de peças), jogam Tetris em tempo real com interface SDL, enviam linhas de lixo um ao outro e a partida termina para os dois ao mesmo tempo quando alguém perde.

## Tecnologias

- C++17.
- CMake 3.16 ou superior e Make.
- Sockets POSIX para comunicação UDP e TCP.
- Pthreads para execução concorrente e sincronização da fila de eventos.
- **SDL2** para a interface gráfica: tabuleiro do jogador, tabuleiro do adversário, pontuação, linhas, nível, lixo pendente e próxima peça (sem imagens nem fontes externas; tudo é desenhado por código).

## Dependências no Ubuntu

Para compilar e executar a versão atual:

```bash
sudo apt update
sudo apt install build-essential cmake libsdl2-dev
```

O pacote `build-essential` inclui o compilador C++ e o Make. As threads POSIX são detectadas pelo CMake com `find_package(Threads REQUIRED)`; não é necessário instalar um pacote chamado `pthread`.

As instruções abaixo são destinadas ao Linux, pois o código utiliza APIs POSIX. No Windows, use um ambiente Linux, como o WSL, ou uma máquina virtual.

### Interface gráfica com SDL

O cliente usa **SDL2** (`libsdl2-dev`, já incluído no comando acima) e o CMake a encontra com `find_package(SDL2 REQUIRED)`. No WSL com Windows 11 (ou Windows 10 atualizado) a janela abre pelo WSLg.

## Abrir e compilar no CLion

1. No CLion, selecione **File > Open** e abra a pasta `project` do repositório. Como ela contém `CMakeLists.txt`, o carregamento do projeto CMake normalmente é automático.
2. Se aparecer um aviso de que o projeto CMake não está carregado, use a opção de selecionar o arquivo de configuração, caso oferecida pelo aviso, e escolha `project/CMakeLists.txt`.
3. Como alternativa documentada, clique com o botão direito em `CMakeLists.txt` na árvore do projeto e selecione **Load CMake Project**. Também é possível abrir diretamente esse arquivo por **File > Open** e escolher **Open as Project**.
4. Aguarde a configuração do CMake. Por padrão, os arquivos de compilação ficam em uma pasta como `project/cmake-build-debug`, criada conforme o perfil ativo. Confira ou altere o caminho em **Settings > Build, Execution, Deployment > CMake > Build directory**.
5. Selecione `tetris_server` ou `tetris_client` nas configurações de execução e use **Build**, **Run** ou **Debug**. Inicie o servidor antes dos clientes.

Não é necessário criar manualmente a pasta `build` nesse fluxo. No Windows, configure uma toolchain Linux/WSL para o código POSIX deste projeto. Para testar em terminais separados, use os executáveis gerados no diretório de build do CLion.

Referências: [carregar um projeto CMake](https://www.jetbrains.com/help/clion/reloading-project.html), [abrir pelo CMakeLists.txt](https://www.jetbrains.com/help/clion/cmakelists-txt-file.html) e [diretório de build do CLion](https://www.jetbrains.com/help/clion/cmake0.html).

## Compilar manualmente, sem o CLion

Abra um terminal na raiz do repositório `tetris-multiplayer` e execute:

```bash
cd project
mkdir -p build
cd build
cmake ..
make
```

**A pasta `build` deve ficar dentro de `project`**, pois o arquivo `CMakeLists.txt` está em `project/CMakeLists.txt`. Dessa forma, `cmake ..` encontra a configuração no diretório pai.

O CMake configura a compilação, e o Make gera os executáveis na pasta `project/build`:

- `tetris_server`: servidor (anúncio UDP, semente, repasse de ataques, fim de jogo).
- `tetris_client`: cliente gráfico do jogo.
- `tetris_tests`: testes automáticos da lógica do Tetris (`./tetris_tests`).

### Recompilar após alterações

Sempre que alterar o código, execute novamente dentro de `project/build`:

```bash
make
```

Pare e reinicie os programas para executar os binários atualizados. Não é necessário recriar a pasta `build` a cada alteração.

Se modificar a configuração do projeto ou adicionar arquivos ao `CMakeLists.txt`, você pode atualizar explicitamente a configuração antes de compilar:

```bash
cmake ..
make
```

## Executar

Compile o projeto antes de iniciar. Em cada terminal abaixo, entre na pasta `project/build` do repositório.

### 1. Iniciar o servidor

No primeiro terminal:

```bash
./tetris_server
```

O servidor anuncia sua disponibilidade por broadcast UDP na porta **5001** (enquanto houver vaga) e aceita conexões TCP na porta **5002**. Ele aceita 2 jogadores por vez, permite jogar várias partidas seguidas e continua no ar quando alguém sai (a vaga volta a ser anunciada).

### 2. Iniciar os clientes

Em um segundo terminal:

```bash
./tetris_client
```

Em um terceiro terminal, execute o mesmo comando para iniciar o outro cliente:

```bash
./tetris_client
```

Os clientes descobrem o IP e a porta TCP do servidor por meio do anúncio UDP. Se o broadcast não funcionar na rede (ou no WSL entre máquinas diferentes), informe o IP do servidor: `./tetris_client 192.168.0.10`.

Cada janela mostra "AGUARDANDO OPONENTE..." até os dois estarem conectados; então começa uma contagem de 3 segundos e a partida inicia.

Para testar em máquinas diferentes, compile o projeto em cada uma e mantenha-as na mesma rede local, com suporte à passagem do broadcast. Se houver firewall, permita a recepção UDP na porta 5001 nos clientes e TCP na porta 5002 no servidor. Redes virtuais ou configurações com isolamento podem impedir a descoberta.

Para encerrar, feche a janela do cliente ou pressione **Esc**; no servidor, use **Ctrl+C** (ou o botão Stop do CLion).

## Limitações conhecidas

- A descoberta por broadcast UDP só funciona dentro da mesma rede local (e, no WSL2 em modo NAT, só entre processos da mesma máquina). Entre máquinas diferentes, use `./tetris_client <ip-do-servidor>` e libere a porta TCP 5002 no servidor.
- O servidor aceita exatamente dois jogadores por vez; uma terceira conexão é recusada.
- O jogo usa rotação simples com "wall kick" básico (não é o SRS oficial) e não há som.

## Como jogar

| Tecla | Ação |
|---|---|
| Setas ← → | mover (segurando, repete) |
| Seta ↑ ou X | girar no sentido horário |
| Z | girar no sentido anti-horário |
| Seta ↓ | descer mais rápido |
| Espaço | queda instantânea (hard drop) |
| Enter | jogar de novo (depois do fim da partida) |
| Esc | sair |

**Ataque:** limpar 2 linhas de uma vez envia 1 linha de lixo ao adversário; 3 linhas enviam 2; 4 linhas (Tetris) enviam 4. As linhas de lixo (cinza, com um buraco aleatório sorteado pelo servidor) entram na base do tabuleiro do adversário quando ele travar a peça atual. A barra vermelha ao lado do seu tabuleiro mostra o lixo pendente.

**Derrota:** perde quem não conseguir posicionar a nova peça (ou for empurrado para fora do topo pelo lixo). O servidor avisa os dois ao mesmo tempo. Se um jogador sair no meio da partida, o outro vence.

### Jogar sozinho para testar

Abra o servidor, um cliente normal e um cliente-robô: `./tetris_client --bot` (use `--bot=0.4` para um robô que erra mais).

## Protocolo (resumo)

Todas as mensagens têm tamanho fixo (`Message`: tipo + 256 bytes de payload) e trafegam em TCP. Veja `project/common/types.h`.

| Tipo | Sentido | Conteúdo |
|---|---|---|
| `MSG_JOIN` | cliente → servidor | "estou pronto" |
| `MSG_START` | servidor → os dois | semente + id do jogador (0 ou 1) |
| `MSG_GAME_STATE` | cliente → servidor → outro | tabuleiro (10x20) + pontos + linhas (10 vezes por segundo) |
| `MSG_ATTACK` | cliente → servidor → outro | nº de linhas de lixo (servidor valida 1 a 4 e sorteia o buraco) |
| `MSG_GAME_OVER` | cliente → servidor | "perdi" |
| `MSG_GAME_END` | servidor → os dois | id do vencedor |
| `MSG_DISCONNECT` | cliente → servidor | "estou saindo" |

A sequência de peças usa "7-bag" com `std::mt19937` inicializado pela semente do servidor, com embaralhamento próprio (Fisher-Yates), então os dois clientes recebem exatamente a mesma ordem.

## Organização do código

- `project/common/types.h`: protocolo compartilhado.
- `project/server/`: `server.cpp` (conexões, anúncio, tratamento das mensagens).
- `project/client/src/game.cpp`: regras do Tetris (sem SDL e sem rede).
- `project/client/src/render.cpp`: desenho com SDL.
- `project/client/src/client.cpp`: rede do cliente (threads de conexão, envio e recepção).
- `project/client/src/main.cpp`: laço principal, teclado e estados da partida.
- `project/client/src/bot.cpp`: jogador automático opcional.
- `project/tests/game_test.cpp`: testes da lógica.
