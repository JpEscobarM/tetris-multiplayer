// Testes da logica do Tetris (sem SDL, sem rede). Rode: ./tetris_tests
#include "game.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>

static int falhas = 0;
#define CHECK(cond, msg) do { if (!(cond)) { printf("  FALHOU: %s\n", msg); falhas++; } else printf("  ok: %s\n", msg); } while (0)

// Monta o tabuleiro por baixo, direto na struct (teste de caixa branca via snapshot nao serve; usamos jogo real)
// Para montar situacoes, jogamos pecas com hard_drop ate chegar la.

static void teste_sequencia() {
    printf("[sequencia de pecas]\n");
    game::PieceBag a(1234), b(1234), c(999);
    bool igual = true, diferente = false;
    for (int i = 0; i < 70; i++) {
        int x = a.next(), y = b.next(), z = c.next();
        if (x != y) igual = false;
        if (x != z) diferente = true;
    }
    CHECK(igual, "mesma semente => mesma sequencia (70 pecas)");
    CHECK(diferente, "sementes diferentes => sequencias diferentes");

    game::PieceBag d(7);
    int cont[7] = {0};
    for (int i = 0; i < 7; i++) cont[d.next()]++;
    bool saco = true;
    for (int i = 0; i < 7; i++) if (cont[i] != 1) saco = false;
    CHECK(saco, "cada 7 pecas contem as 7 diferentes (7-bag)");

    game::PieceBag e(5);
    int p0 = e.peek(0), p1 = e.peek(1);
    CHECK(e.next() == p0 && e.next() == p1, "peek nao consome pecas");
}

static void teste_movimento() {
    printf("[movimento e rotacao]\n");
    game::Game g;
    g.start(42);
    int x0 = g.current().x;
    CHECK(g.move(-1) && g.current().x == x0 - 1, "move para a esquerda");
    int n = 0;
    while (g.move(-1)) n++;
    CHECK(g.current().x + 0 >= -1 && !g.move(-1), "nao atravessa a parede esquerda");
    while (g.move(1)) {}
    CHECK(!g.move(1), "nao atravessa a parede direita");
    int r0 = g.current().rot;
    bool girou = g.rotate(1);
    CHECK(!girou || g.current().rot == ((r0 + 1) & 3), "rotacao horaria muda rot em +1");
    // 4 rotacoes voltam ao formato original (testado em uma peca T)
    const game::Cells& a = game::piece_cells(2, 0);
    const game::Cells& b = game::piece_cells(2, 4);   // rot & 3 => 0
    CHECK(std::memcmp(&a, &b, sizeof(a)) == 0, "rot 4 == rot 0");
    (void)n;
}

static void teste_hard_drop_e_game_over() {
    printf("[hard drop e game over]\n");
    game::Game g;
    g.start(3);
    int antes = g.score();
    g.hard_drop();
    CHECK(g.score() > antes, "hard drop pontua");
    CHECK(g.pieces_spawned() == 2, "nova peca entra apos travar");
    int guard = 0;
    while (!g.over() && guard++ < 200) g.hard_drop();   // sempre no meio: empilha ate o topo
    CHECK(g.over(), "empilhar ate o topo => game over (nova peca nao cabe)");
    CHECK(!g.move(1) && !g.rotate(1), "apos game over nao responde a comandos");
}

// Joga pecas do tipo I deitadas para limpar linhas e medir o ataque.
// Usamos a propria sequencia: procuramos pecas I e as posicionamos; outras pecas vao para a direita.
static void teste_linhas_e_ataque() {
    printf("[linhas completas e ataque]\n");
    // Estrategia: preencher a linha de baixo com pecas I (4 de largura) nas colunas 0-3 e 4-7,
    // e O (2 de largura) nas colunas 8-9. Escolhemos a semente e descartamos pecas inuteis no canto.
    // Em vez disso, verificamos a tabela de ataque via varios cenarios gerados por busca.
    int melhor = 0, vistoAtaque[5] = {0};
    for (uint32_t seed = 1; seed < 4000; seed++) {
        game::Game g;
        g.start(seed);
        int lim = 0;
        // bot guloso minimo: coloca cada peca na coluna que gera menos buracos (so contamos clears)
        while (!g.over() && lim++ < 60) {
            int bestScore = -1000, bestRot = 0, bestX = 0;
            for (int rot = 0; rot < 4; rot++) {
                const game::Cells& c = game::piece_cells(g.current().type, rot);
                for (int x = -2; x < game::COLS; x++) {
                    if (g.collides(g.current().type, rot, x, g.current().y)) continue;
                    int y = g.current().y;
                    while (!g.collides(g.current().type, rot, x, y + 1)) y++;
                    // preferir posicoes baixas e que preencham linhas
                    int s = 0, rowCount[game::ROWS] = {0};
                    for (int i = 0; i < 4; i++) rowCount[y + c.y[i]]++;
                    for (int r = 0; r < game::ROWS; r++) {
                        int cnt = rowCount[r];
                        for (int xx = 0; xx < game::COLS; xx++) if (g.cell(xx, r)) cnt++;
                        s += cnt * cnt * (r + 1) / 4;
                    }
                    if (s > bestScore) { bestScore = s; bestRot = rot; bestX = x; }
                }
            }
            while (g.current().rot != bestRot && g.rotate(1)) {}
            while (g.current().x < bestX && g.move(1)) {}
            while (g.current().x > bestX && g.move(-1)) {}
            int l0 = g.lines();
            g.hard_drop();
            int cl = g.lines() - l0;
            int atk = g.take_attack();
            if (cl >= 0 && cl <= 4) {
                int esperado = (cl == 2) ? 1 : (cl == 3) ? 2 : (cl == 4) ? 4 : 0;
                if (atk != esperado) { printf("  FALHOU: %d linhas deram ataque %d (esperado %d)\n", cl, atk, esperado); falhas++; return; }
                vistoAtaque[cl]++;
                if (cl > melhor) melhor = cl;
            }
        }
    }
    printf("  ocorrencias de limpeza: 1=%d 2=%d 3=%d 4=%d\n", vistoAtaque[1], vistoAtaque[2], vistoAtaque[3], vistoAtaque[4]);
    CHECK(vistoAtaque[1] > 0 && vistoAtaque[2] > 0, "limpeza de 1 e 2 linhas observadas, ataque 0 e 1 corretos");
    CHECK(vistoAtaque[3] > 0, "triple observado => ataque 2");
    CHECK(vistoAtaque[4] > 0, "tetris observado => ataque 4");
}

static void teste_lixo() {
    printf("[linhas de lixo recebidas]\n");
    game::Game g;
    g.start(11);
    g.receive_garbage(2, 3);
    CHECK(g.pending_garbage() == 2, "lixo fica pendente enquanto a peca nao trava");
    g.hard_drop();
    CHECK(g.pending_garbage() == 0, "lixo aplicado quando a peca trava");
    bool ok = true;
    for (int r = game::ROWS - 2; r < game::ROWS; r++)
        for (int x = 0; x < game::COLS; x++) {
            int v = g.cell(x, r);
            if (x == 3 && v == game::GARBAGE) ok = false;       // buraco na coluna 3
            if (x != 3 && v == 0 && r == game::ROWS - 1) ok = false;
        }
    CHECK(ok, "2 linhas cinzas na base, com buraco na coluna pedida");
    StatePayload sp;
    g.snapshot(sp);
    CHECK(sp.cells[(game::ROWS - 1) * game::COLS + 0] != 0 || sp.cells[(game::ROWS - 1) * game::COLS + 1] != 0, "snapshot reflete o tabuleiro");

    // lixo suficiente para estourar o topo
    game::Game h;
    h.start(12);
    for (int i = 0; i < 12 && !h.over(); i++) { h.receive_garbage(4, 0); h.hard_drop(); }
    CHECK(h.over(), "lixo que empurra blocos para fora do topo => game over");
}

int main() {
    teste_sequencia();
    teste_movimento();
    teste_hard_drop_e_game_over();
    teste_linhas_e_ataque();
    teste_lixo();
    printf("\n%s (%d falha(s))\n", falhas ? "REPROVADO" : "TUDO OK", falhas);
    return falhas ? 1 : 0;
}
