#include "render.hpp"
#include <cctype>
#include <cstdio>
#include <cstring>

namespace render {

namespace {

// ------------------------------------------------------------ fonte 5x7
struct Glyph { char ch; uint8_t rows[7]; };
const Glyph FONT[] = {
    {' ', {0, 0, 0, 0, 0, 0, 0}},
    {'0', {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E}},
    {'1', {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E}},
    {'2', {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F}},
    {'3', {0x1E, 0x01, 0x01, 0x0E, 0x01, 0x01, 0x1E}},
    {'4', {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02}},
    {'5', {0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E}},
    {'6', {0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E}},
    {'7', {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08}},
    {'8', {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E}},
    {'9', {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C}},
    {'A', {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}},
    {'B', {0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E}},
    {'C', {0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E}},
    {'D', {0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E}},
    {'E', {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F}},
    {'F', {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10}},
    {'G', {0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0F}},
    {'H', {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}},
    {'I', {0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E}},
    {'J', {0x07, 0x02, 0x02, 0x02, 0x02, 0x12, 0x0C}},
    {'K', {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11}},
    {'L', {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F}},
    {'M', {0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11}},
    {'N', {0x11, 0x11, 0x19, 0x15, 0x13, 0x11, 0x11}},
    {'O', {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}},
    {'P', {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10}},
    {'Q', {0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D}},
    {'R', {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11}},
    {'S', {0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E}},
    {'T', {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04}},
    {'U', {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}},
    {'V', {0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04}},
    {'W', {0x11, 0x11, 0x11, 0x15, 0x15, 0x15, 0x0A}},
    {'X', {0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11}},
    {'Y', {0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04}},
    {'Z', {0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F}},
    {'.', {0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C}},
    {':', {0x00, 0x0C, 0x0C, 0x00, 0x0C, 0x0C, 0x00}},
    {'!', {0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x04}},
    {'-', {0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00}},
    {'/', {0x01, 0x02, 0x02, 0x04, 0x08, 0x08, 0x10}},
    {'?', {0x0E, 0x11, 0x01, 0x02, 0x04, 0x00, 0x04}},
};

const Glyph* find_glyph(char c) {
    c = (char)std::toupper((unsigned char)c);
    for (const Glyph& g : FONT) if (g.ch == c) return &g;
    return &FONT[0];
}

// ----------------------------------------------------------------- cores
const SDL_Color BG       = {18, 20, 28, 255};
const SDL_Color PANEL    = {28, 31, 43, 255};
const SDL_Color GRID     = {38, 42, 56, 255};
const SDL_Color WHITE    = {235, 238, 245, 255};
const SDL_Color DIM      = {140, 148, 170, 255};
const SDL_Color ACCENT   = {255, 205, 80, 255};
const SDL_Color RED      = {235, 70, 70, 255};
const SDL_Color GREEN    = {90, 220, 120, 255};

SDL_Color piece_color(int v) {   // v: 1..7 pecas, 8 lixo
    static const SDL_Color C[9] = {
        {0, 0, 0, 255},
        {0, 220, 230, 255},    // I
        {240, 220, 0, 255},    // O
        {170, 70, 230, 255},   // T
        {60, 210, 70, 255},    // S
        {230, 60, 60, 255},    // Z
        {60, 90, 235, 255},    // J
        {245, 150, 30, 255},   // L
        {125, 130, 140, 255},  // lixo
    };
    return C[(v < 0 || v > 8) ? 0 : v];
}

void set(SDL_Renderer* r, SDL_Color c) { SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a); }

void fill(SDL_Renderer* r, int x, int y, int w, int h, SDL_Color c) {
    SDL_Rect rc = {x, y, w, h};
    set(r, c);
    SDL_RenderFillRect(r, &rc);
}

void block(SDL_Renderer* r, int x, int y, int s, SDL_Color c) {
    fill(r, x, y, s, s, c);
    int b = s >= 20 ? 3 : 2;
    SDL_Color hi = {(Uint8)(c.r + (255 - c.r) / 2), (Uint8)(c.g + (255 - c.g) / 2), (Uint8)(c.b + (255 - c.b) / 2), 255};
    SDL_Color lo = {(Uint8)(c.r / 2), (Uint8)(c.g / 2), (Uint8)(c.b / 2), 255};
    fill(r, x, y, s, b, hi);
    fill(r, x, y, b, s, hi);
    fill(r, x, y + s - b, s, b, lo);
    fill(r, x + s - b, y, b, s, lo);
}

void center_text(SDL_Renderer* r, int cx, int y, int scale, const char* t, SDL_Color c) {
    draw_text(r, cx - text_width(t, scale) / 2, y, scale, t, c);
}

// Desenha um tabuleiro 10x20. cells: 200 valores (0..8).
void draw_board(SDL_Renderer* r, int x0, int y0, int cs, const uint8_t* cells) {
    fill(r, x0 - 4, y0 - 4, game::COLS * cs + 8, game::ROWS * cs + 8, PANEL);
    fill(r, x0, y0, game::COLS * cs, game::ROWS * cs, BG);
    set(r, GRID);
    for (int i = 1; i < game::COLS; i++) SDL_RenderDrawLine(r, x0 + i * cs, y0, x0 + i * cs, y0 + game::ROWS * cs);
    for (int j = 1; j < game::ROWS; j++) SDL_RenderDrawLine(r, x0, y0 + j * cs, x0 + game::COLS * cs, y0 + j * cs);
    for (int y = 0; y < game::ROWS; y++)
        for (int x = 0; x < game::COLS; x++) {
            int v = cells[y * game::COLS + x];
            if (v) block(r, x0 + x * cs, y0 + y * cs, cs, piece_color(v));
        }
}

void draw_label_value(SDL_Renderer* r, int x, int y, const char* label, int value) {
    char buf[32];
    draw_text(r, x, y, 2, label, DIM);
    std::snprintf(buf, sizeof(buf), "%d", value);
    draw_text(r, x, y + 20, 3, buf, WHITE);
}

void overlay(SDL_Renderer* r, int x, int y, int w, int h) {
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 0, 0, 0, 170);
    SDL_Rect rc = {x, y, w, h};
    SDL_RenderFillRect(r, &rc);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}

// posicoes fixas do layout
constexpr int OWN_X = 50, OWN_Y = 80, OWN_CS = 30;
constexpr int OPP_X = 600, OPP_Y = 80, OPP_CS = 25;
constexpr int PANEL_X = 395;

}  // namespace

int text_width(const char* text, int scale) {
    int n = (int)std::strlen(text);
    return n == 0 ? 0 : n * 6 * scale - scale;
}

void draw_text(SDL_Renderer* r, int x, int y, int scale, const char* text, SDL_Color color) {
    set(r, color);
    for (const char* p = text; *p; p++, x += 6 * scale) {
        const Glyph* g = find_glyph(*p);
        for (int row = 0; row < 7; row++)
            for (int col = 0; col < 5; col++)
                if (g->rows[row] & (1 << (4 - col))) {
                    SDL_Rect rc = {x + col * scale, y + row * scale, scale, scale};
                    SDL_RenderFillRect(r, &rc);
                }
    }
}

void draw(SDL_Renderer* r, const View& v) {
    set(r, BG);
    SDL_RenderClear(r);

    center_text(r, WIN_W / 2, 16, 3, "TETRIS MULTIPLAYER", ACCENT);

    const bool inGame = v.game && (v.screen == Screen::Countdown || v.screen == Screen::Playing ||
                                   v.screen == Screen::WaitingEnd || v.screen == Screen::Finished);

    // ---------------------------------------------------------------- meu tabuleiro
    draw_text(r, OWN_X, 56, 2, "VOCE", GREEN);
    uint8_t own[game::COLS * game::ROWS] = {0};
    if (inGame) {
        StatePayload sp;
        v.game->snapshot(sp);
        std::memcpy(own, sp.cells, sizeof(own));
    }
    draw_board(r, OWN_X, OWN_Y, OWN_CS, own);

    if (inGame && v.game->has_piece() && v.game->active()) {   // sombra (onde a peca vai cair)
        const game::Piece& p = v.game->current();
        const game::Cells& c = game::piece_cells(p.type, p.rot);
        int gy = v.game->ghost_y();
        SDL_Color col = piece_color(p.type + 1);
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
        for (int i = 0; i < 4; i++) {
            int cx = p.x + c.x[i], cy = gy + c.y[i];
            if (cy < 0 || cy >= game::ROWS || cy == p.y + c.y[i]) continue;
            SDL_SetRenderDrawColor(r, col.r, col.g, col.b, 170);
            SDL_Rect rc = {OWN_X + cx * OWN_CS + 2, OWN_Y + cy * OWN_CS + 2, OWN_CS - 4, OWN_CS - 4};
            SDL_RenderDrawRect(r, &rc);
        }
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
    }

    // barra de lixo pendente (vermelha, ao lado do tabuleiro)
    if (inGame) {
        int pend = v.game->pending_garbage();
        if (pend > game::ROWS) pend = game::ROWS;
        fill(r, OWN_X + game::COLS * OWN_CS + 10, OWN_Y, 8, game::ROWS * OWN_CS, PANEL);
        if (pend > 0)
            fill(r, OWN_X + game::COLS * OWN_CS + 10, OWN_Y + (game::ROWS - pend) * OWN_CS, 8, pend * OWN_CS, RED);
    }

    // ----------------------------------------------------------------- painel central
    draw_text(r, PANEL_X, 80, 2, "PROXIMA", DIM);
    fill(r, PANEL_X, 104, 110, 90, PANEL);
    if (inGame && v.game->active()) {
        int t = const_cast<game::Game*>(v.game)->next_type(0);
        const game::Cells& c = game::piece_cells(t, 0);
        int minx = 9, maxx = -9, miny = 9, maxy = -9;
        for (int i = 0; i < 4; i++) {
            minx = c.x[i] < minx ? c.x[i] : minx; maxx = c.x[i] > maxx ? c.x[i] : maxx;
            miny = c.y[i] < miny ? c.y[i] : miny; maxy = c.y[i] > maxy ? c.y[i] : maxy;
        }
        const int s = 22;
        int ox = PANEL_X + (110 - (maxx - minx + 1) * s) / 2 - minx * s;
        int oy = 104 + (90 - (maxy - miny + 1) * s) / 2 - miny * s;
        for (int i = 0; i < 4; i++) block(r, ox + c.x[i] * s, oy + c.y[i] * s, s, piece_color(t + 1));
    }
    if (inGame) {
        draw_label_value(r, PANEL_X, 220, "PONTOS", v.game->score());
        draw_label_value(r, PANEL_X, 280, "LINHAS", v.game->lines());
        draw_label_value(r, PANEL_X, 340, "NIVEL", v.game->level() + 1);
        draw_label_value(r, PANEL_X, 400, "LIXO", v.game->pending_garbage());
    }
    draw_text(r, PANEL_X, 480, 2, "CONTROLES", DIM);
    draw_text(r, PANEL_X, 506, 2, "SETAS MOVE", WHITE);
    draw_text(r, PANEL_X, 526, 2, "CIMA GIRA", WHITE);
    draw_text(r, PANEL_X, 546, 2, "Z GIRA", WHITE);
    draw_text(r, PANEL_X, 566, 2, "BAIXO DESCE", WHITE);
    draw_text(r, PANEL_X, 586, 2, "ESPACO QUEDA", WHITE);
    draw_text(r, PANEL_X, 606, 2, "ESC SAI", WHITE);

    // ------------------------------------------------------------ tabuleiro do adversario
    draw_text(r, OPP_X, 56, 2, "OPONENTE", RED);
    uint8_t opp[game::COLS * game::ROWS] = {0};
    int oppScore = 0, oppLines = 0;
    if (v.opponent && inGame) {
        std::memcpy(opp, v.opponent->cells, sizeof(opp));
        oppScore = v.opponent->score;
        oppLines = v.opponent->lines;
    }
    draw_board(r, OPP_X, OPP_Y, OPP_CS, opp);
    draw_label_value(r, OPP_X, 600, "PONTOS", oppScore);
    draw_label_value(r, OPP_X + 130, 600, "LINHAS", oppLines);

    // ------------------------------------------------------------------ mensagens
    const int bx = OWN_X, bw = game::COLS * OWN_CS, by = OWN_Y, bh = game::ROWS * OWN_CS;
    const int cx = bx + bw / 2, cy = by + bh / 2;
    char buf[64];
    switch (v.screen) {
        case Screen::Connecting:
            overlay(r, bx, by, bw, bh);
            center_text(r, cx, cy - 30, 3, "PROCURANDO", WHITE);
            center_text(r, cx, cy, 3, "SERVIDOR...", WHITE);
            break;
        case Screen::Lobby:
            overlay(r, bx, by, bw, bh);
            center_text(r, cx, cy - 30, 3, "AGUARDANDO", WHITE);
            center_text(r, cx, cy, 3, "OPONENTE...", WHITE);
            break;
        case Screen::Countdown:
            overlay(r, bx, by, bw, bh);
            center_text(r, cx, cy - 20, 3, "PREPARE-SE", WHITE);
            std::snprintf(buf, sizeof(buf), "%d", v.countdown);
            center_text(r, cx, cy + 20, 8, buf, ACCENT);
            break;
        case Screen::Finished:
            overlay(r, bx, by, bw, bh);
            center_text(r, cx, cy - 50, 3, v.won ? "VOCE VENCEU!" : "VOCE PERDEU", v.won ? GREEN : RED);
            center_text(r, cx, cy, 2, "ENTER JOGA DE NOVO", WHITE);
            center_text(r, cx, cy + 24, 2, "ESC SAI", DIM);
            break;
        case Screen::WaitingEnd:
            overlay(r, bx, by, bw, bh);
            center_text(r, cx, cy, 3, "FIM DE JOGO", RED);
            break;
        case Screen::Disconnected:
            overlay(r, bx, by, bw, bh);
            center_text(r, cx, cy - 30, 3, "CONEXAO", RED);
            center_text(r, cx, cy, 3, "PERDIDA", RED);
            center_text(r, cx, cy + 40, 2, "ESC SAI", DIM);
            break;
        case Screen::Playing:
            break;
    }
}

}  // namespace render
