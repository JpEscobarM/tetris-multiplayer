# Requisitos e decisões do projeto Tetris

Registro solicitado pelo usuário para referência em conversas futuras.
Fonte dos requisitos: `desricao-projeto.pdf` (duas páginas).

## Requisitos do trabalho

- Tetris funcional e competitivo para dois jogadores via rede, em tempo real.
- Cada jogador possui seu próprio tabuleiro; suas ações podem afetar o adversário.
- Ambos recebem exatamente a mesma sequência de tetriminos, na mesma ordem.
- Controles individuais pelo teclado: movimento lateral, rotação e queda rápida (Hard Drop).
- Limpar duas linhas simultaneamente envia uma linha de lixo; três enviam duas; quatro enviam quatro.
- Linhas de lixo são cinzas, com um buraco aleatório, e entram na base do tabuleiro adversário.
- Perde quem acumular blocos no topo e não conseguir posicionar a nova peça; o adversário vence.
- A comunicação da partida deve usar sockets TCP.
- O servidor gera a semente aleatória inicial para sincronizar a sequência de peças.
- O servidor intermedeia os ataques entre jogadores.
- O servidor monitora Game Over para encerrar a partida simultaneamente.
- A interface mostra claramente os dois tabuleiros (próprio e adversário), a pontuação e a próxima peça. Pode usar uma janela ou janelas separadas.
- Grupos de até dois integrantes.

## Decisões informadas pelo usuário

- Interface gráfica com SDL. A versão principal (SDL2 ou SDL3) ainda não foi confirmada.
- O usuário está aprendendo e deseja ajuda com explicações e exemplos ligados ao código.

## Contexto e limites

- Projeto em C++ com CMake e sockets/threads POSIX.
- Descoberta por broadcast UDP na porta 5001 e conexão TCP na porta 5002 são escolhas da implementação, não exigências do PDF.
- O documento não define desconexão, reconexão ou substituição de jogadores; não tratar uma política sugerida como requisito obrigatório.
- A análise inicial foi solicitada sem alterações de código. Este registro foi autorizado posteriormente; não implica autorização para implementar funcionalidades sem solicitação.

