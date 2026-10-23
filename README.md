# Tetris Multiplayer

Projeto de Tetris competitivo para dois jogadores, desenvolvido para aprender C++, interfaces gráficas, sockets e programação concorrente.

A implementação atual permite descobrir o servidor por UDP, conectar clientes por TCP e enviar textos digitados no terminal por meio de uma fila de eventos. A lógica completa da partida e a interface gráfica ainda estão em desenvolvimento.

## Tecnologias

- C++17.
- CMake 3.16 ou superior e Make.
- Sockets POSIX para comunicação UDP e TCP.
- Pthreads para execução concorrente e sincronização da fila de eventos.
- **SDL para a futura interface gráfica**, que deverá exibir os tabuleiros dos dois jogadores, a pontuação e a próxima peça.

## Dependências no Ubuntu

Para compilar e executar a versão atual:

```bash
sudo apt update
sudo apt install build-essential cmake
```

O pacote `build-essential` inclui o compilador C++ e o Make. As threads POSIX são detectadas pelo CMake com `find_package(Threads REQUIRED)`; não é necessário instalar um pacote chamado `pthread`.

As instruções abaixo são destinadas ao Linux, pois o código utiliza APIs POSIX. No Windows, use um ambiente Linux, como o WSL, ou uma máquina virtual.

### Interface gráfica com SDL

A interface gráfica deverá ser desenvolvida com SDL. Ela ainda não está integrada ao CMake e não é necessária para executar os testes atuais pelo terminal.

A versão principal da SDL ainda precisa ser confirmada. Para uma futura integração com SDL3, em versões do Ubuntu que disponibilizem o pacote nos repositórios habilitados:

```bash
sudo apt install libsdl3-dev
```

Se o pacote não estiver disponível, consulte a [documentação oficial de instalação e integração com CMake](https://wiki.libsdl.org/SDL3/INTRO-cmake). Instalar a biblioteca não a integra automaticamente ao projeto; essa configuração será adicionada durante o desenvolvimento da interface.

Referências: [build-essential no Ubuntu](https://packages.ubuntu.com/build-essential) e [pacote de desenvolvimento SDL3 no Ubuntu](https://packages.ubuntu.com/stonking/libsdl3-dev).

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

- `tetris_server`: servidor de descoberta e atendimento dos clientes.
- `tetris_client`: cliente que descobre o servidor e envia eventos.

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

O servidor anuncia sua disponibilidade por broadcast UDP na porta **5001** e aceita conexões TCP na porta **5002**. O limite atual é de dois atendimentos de clientes.

### 2. Iniciar os clientes

Em um segundo terminal:

```bash
./tetris_client
```

Em um terceiro terminal, execute o mesmo comando para iniciar o outro cliente:

```bash
./tetris_client
```

Os clientes descobrem o IP e a porta TCP do servidor por meio do anúncio UDP; não há argumentos de endereço na linha de comando.

Para testar em máquinas diferentes, compile o projeto em cada uma e mantenha-as na mesma rede local, com suporte à passagem do broadcast. Se houver firewall, permita a recepção UDP na porta 5001 nos clientes e TCP na porta 5002 no servidor. Redes virtuais ou configurações com isolamento podem impedir a descoberta.

### 3. Testar o envio de eventos

Após a conexão, digite uma mensagem no terminal de um cliente e pressione **Enter**. A thread produtora insere o texto na fila circular; a consumidora retira o evento e o envia ao servidor por TCP. O servidor imprime os dados recebidos para depuração.

Esse teste verifica o fluxo de comunicação; ainda não representa uma partida de Tetris. Para encerrar os processos manualmente, use **Ctrl+C** em cada terminal.

## Limitações atuais

- O encerramento das threads produtoras e consumidoras ainda não é coordenado; EOF na entrada não garante o término do cliente.
- A aceitação conta dois atendimentos ao longo da execução e não implementa substituição automática de jogadores desconectados. Para repetir uma sessão completa de teste, reinicie servidor e clientes.
- As mensagens de texto são enviadas com terminador `\0`, mas a recepção ainda precisa tratar a separação de mensagens no fluxo TCP. Uma leitura pode conter parte de uma mensagem ou várias mensagens juntas.
- A implementação está em desenvolvimento; compilar não garante que todos os cenários de conexão e desconexão estejam tratados.
