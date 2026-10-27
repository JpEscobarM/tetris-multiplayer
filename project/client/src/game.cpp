#include "game.hpp"
#include <algorithm>
#include <cstring>

namespace game {

namespace {

// Formato de cada peca na rotacao 0 ('X' = quadradinho), dentro de uma caixa n x n.
struct Shape { int n; const char* rows; };
const Shape SHAPES[NUM_PIECES] = {
    {4, "...." "XXXX" "...." "...."},   // I
    {2, "XX"   "XX"},                   // O
    {3, ".X."  "XXX" "..."},            // T
    {3, ".XX"  "XX." "..."},            // S
    {3, "XX."  ".XX" "..."},            // Z
    {3, "X.."  "XXX" "..."},            // J
    {3, "..X"  "XXX" "..."},            // L
};

struct Tables {
    Cells cells[NUM_PIECES][4];
    int minY[NUM_PIECES];   // primeira linha ocupada na rotacao 0
    int size[NUM_PIECES];
};

Tables build_tables() {
    Tables t{};
    for (int p = 0; p < NUM_PIECES; p++) {
        const int n = SHAPES[p].n;
        char g[4][4];
        for (int r = 0; r < 4; r++) for (int c = 0; c < 4; c++) g[r][c] = '.';
        for (int r = 0; r < n; r++) for (int c = 0; c < n; c++) g[r][c] = SHAPES[p].rows[r * n + c];

        for (int rot = 0; rot < 4; rot++) {
            int k = 0;
            for (int r = 0; r < n; r++)
                for (int c = 0; c < n; c++)
                    if (g[r][c] == 'X') { t.cells[p][rot].x[k] = c; t.cells[p][rot].y[k] = r; k++; }

            // gira 90 graus no sentido horario: novo[r][c] = antigo[n-1-c][r]
            char ng[4][4];
            for (int r = 0; r < 4; r++) for (int c = 0; c < 4; c++) ng[r][c] = '.';
            for (int r = 0; r < n; r++) for (int c = 0; c < n; c++) ng[r][c] = g[n - 1 - c][r];
            std::memcpy(g, ng, sizeof(g));
        }
        t.size[p] = n;
        t.minY[p] = 99;
        for (int i = 0; i < 4; i++) t.minY[p] = std::min(t.minY[p], t.cells[p][0].y[i]);
    }
    return t;
}

const Tables& tables() {
    static const Tables t = build_tables();
    return t;
}

const int LINE_SCORE[5] = {0, 100, 300, 500, 800};
const int ATTACK_FOR_LINES[5] = {0, 0, 1, 2, 4};   // regra do professor: 2->1, 3->2, 4->4

}  // namespace

const Cells& piece_cells(int type, int rot) { return tables().cells[type][rot & 3]; }

// ---------------------------------------------------------------- PieceBag
void PieceBag::reset(uint32_t seed) {
    rng_.seed(seed);
    queue_.clear();
}

void PieceBag::fill(size_t n) {
    while (queue_.size() < n) {
        int a[NUM_PIECES] = {0, 1, 2, 3, 4, 5, 6};
        // Fisher-Yates "na mao": std::shuffle/distribution nao sao iguais entre compiladores.
        for (int i = NUM_PIECES - 1; i > 0; i--) {
            int j = (int)(rng_() % (uint32_t)(i + 1));
            std::swap(a[i], a[j]);
        }
        for (int i = 0; i < NUM_PIECES; i++) queue_.push_back(a[i]);
    }
}

int PieceBag::next() {
    fill(1);
    int t = queue_.front();
    queue_.pop_front();
    return t;
}

int PieceBag::peek(int i) {
    fill((size_t)i + 1);
    return queue_[(size_t)i];
}

// -------------------------------------------------------------------- Game
void Game::start(uint32_t seed) {
    std::memset(board_, 0, sizeof(board_));
    bag_.reset(seed);
    pending_.clear();
    score_ = lines_ = attackOut_ = gravityAcc_ = 0;
    spawned_ = 0;
    over_ = false;
    hasPiece_ = false;
    active_ = true;
    spawn();
}

bool Game::collides(int type, int rot, int x, int y) const {
    const Cells& c = piece_cells(type, rot);
    for (int i = 0; i < 4; i++) {
        int cx = x + c.x[i], cy = y + c.y[i];
        if (cx < 0 || cx >= COLS || cy >= ROWS) return true;
        if (cy >= 0 && board_[cy][cx]) return true;   // acima do topo (cy < 0) e permitido
    }
    return false;
}

void Game::spawn() {
    const Tables& t = tables();
    cur_.type = bag_.next();
    cur_.rot = 0;
    cur_.x = (COLS - t.size[cur_.type]) / 2;
    cur_.y = -t.minY[cur_.type];       // a primeira linha da peca entra na linha 0
    gravityAcc_ = 0;
    spawned_++;
    if (collides(cur_.type, cur_.rot, cur_.x, cur_.y)) {
        // Regra do professor: nao conseguiu posicionar a nova peca => perdeu.
        over_ = true;
        hasPiece_ = false;
    } else {
        hasPiece_ = true;
    }
}

bool Game::move(int dx) {
    if (!active() || !hasPiece_) return false;
    if (collides(cur_.type, cur_.rot, cur_.x + dx, cur_.y)) return false;
    cur_.x += dx;
    return true;
}

bool Game::rotate(int dir) {
    if (!active() || !hasPiece_) return false;
    static const int KICKS[6][2] = {{0, 0}, {-1, 0}, {1, 0}, {0, -1}, {-2, 0}, {2, 0}};
    int nr = (cur_.rot + dir + 4) & 3;
    for (auto& k : KICKS) {
        if (!collides(cur_.type, nr, cur_.x + k[0], cur_.y + k[1])) {
            cur_.rot = nr;
            cur_.x += k[0];
            cur_.y += k[1];
            return true;
        }
    }
    return false;
}

bool Game::soft_drop() {
    if (!active() || !hasPiece_) return false;
    if (!collides(cur_.type, cur_.rot, cur_.x, cur_.y + 1)) {
        cur_.y++;
        score_ += 1;
        gravityAcc_ = 0;
        return true;
    }
    lock_piece();
    return false;
}

void Game::hard_drop() {
    if (!active() || !hasPiece_) return;
    int d = ghost_y() - cur_.y;
    cur_.y += d;
    score_ += 2 * d;
    lock_piece();
}

int Game::ghost_y() const {
    int y = cur_.y;
    while (!collides(cur_.type, cur_.rot, cur_.x, y + 1)) y++;
    return y;
}

int Game::gravity_interval() const { return std::max(100, 800 - level() * 60); }

void Game::update(int dt_ms) {
    if (!active() || !hasPiece_) return;
    gravityAcc_ += dt_ms;
    int guard = 0;
    while (gravityAcc_ >= gravity_interval() && hasPiece_ && !over_ && guard++ < 10) {
        gravityAcc_ -= gravity_interval();
        if (!collides(cur_.type, cur_.rot, cur_.x, cur_.y + 1)) cur_.y++;
        else lock_piece();
    }
}

void Game::lock_piece() {
    const Cells& c = piece_cells(cur_.type, cur_.rot);
    bool toppedOut = false;
    for (int i = 0; i < 4; i++) {
        int cx = cur_.x + c.x[i], cy = cur_.y + c.y[i];
        if (cy < 0) toppedOut = true;               // travou acima da tela
        else board_[cy][cx] = (uint8_t)(cur_.type + 1);
    }
    hasPiece_ = false;

    // remove linhas completas
    int cleared = 0;
    for (int r = ROWS - 1; r >= 0;) {
        bool full = true;
        for (int x = 0; x < COLS; x++) if (!board_[r][x]) { full = false; break; }
        if (!full) { r--; continue; }
        for (int k = r; k > 0; k--) std::memcpy(board_[k], board_[k - 1], COLS);
        std::memset(board_[0], 0, COLS);
        cleared++;                                   // mesma linha r precisa ser testada de novo
    }
    score_ += LINE_SCORE[cleared] * (level() + 1);
    lines_ += cleared;
    attackOut_ += ATTACK_FOR_LINES[cleared];

    apply_garbage();                                  // lixo recebido entra depois de travar
    if (toppedOut) over_ = true;
    if (!over_) spawn();
}

void Game::receive_garbage(int lines, int holeCol) {
    if (lines <= 0 || !active()) return;
    if (holeCol < 0 || holeCol >= COLS) holeCol = 0;
    pending_.push_back({lines, holeCol});
}

int Game::pending_garbage() const {
    int n = 0;
    for (auto& p : pending_) n += p.first;
    return n;
}

void Game::apply_garbage() {
    for (auto& p : pending_) {
        for (int l = 0; l < p.first; l++) {
            for (int x = 0; x < COLS; x++)
                if (board_[0][x]) { over_ = true; break; }       // empurrou bloco para fora do topo
            for (int r = 0; r < ROWS - 1; r++) std::memcpy(board_[r], board_[r + 1], COLS);
            for (int x = 0; x < COLS; x++) board_[ROWS - 1][x] = (x == p.second) ? 0 : GARBAGE;
        }
    }
    pending_.clear();
}

int Game::take_attack() {
    int a = attackOut_;
    attackOut_ = 0;
    return a;
}

void Game::snapshot(StatePayload& out) const {
    std::memset(&out, 0, sizeof(out));
    for (int y = 0; y < ROWS; y++)
        for (int x = 0; x < COLS; x++) out.cells[y * COLS + x] = board_[y][x];
    if (hasPiece_) {
        const Cells& c = piece_cells(cur_.type, cur_.rot);
        for (int i = 0; i < 4; i++) {
            int cx = cur_.x + c.x[i], cy = cur_.y + c.y[i];
            if (cy >= 0 && cy < ROWS && cx >= 0 && cx < COLS) out.cells[cy * COLS + cx] = (uint8_t)(cur_.type + 1);
        }
    }
    out.score = score_;
    out.lines = lines_;
}

}  // namespace game
