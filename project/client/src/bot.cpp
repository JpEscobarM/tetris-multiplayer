#include "bot.hpp"
#include <vector>

namespace bot {

Bot::Bot(double skill, int stepMs) : skill_(skill), stepMs_(stepMs), rng_(std::random_device{}()) {}

void Bot::plan(game::Game& g) {
    using namespace game;
    struct Option { int rot, x; double score; };
    std::vector<Option> opts;
    const Piece& cur = g.current();

    for (int rot = 0; rot < 4; rot++) {
        const Cells& c = piece_cells(cur.type, rot);
        for (int x = -2; x < COLS; x++) {
            if (g.collides(cur.type, rot, x, cur.y)) continue;
            int y = cur.y;
            while (!g.collides(cur.type, rot, x, y + 1)) y++;

            bool grid[ROWS][COLS];
            for (int r = 0; r < ROWS; r++) for (int k = 0; k < COLS; k++) grid[r][k] = g.cell(k, r) != 0;
            bool fora = false;
            for (int i = 0; i < 4; i++) {
                int cy = y + c.y[i];
                if (cy < 0) fora = true; else grid[cy][x + c.x[i]] = true;
            }
            if (fora) continue;

            // remove linhas completas e mede a qualidade do resultado
            bool g2[ROWS][COLS] = {};
            int lines = 0, dst = ROWS - 1;
            for (int r = ROWS - 1; r >= 0; r--) {
                bool full = true;
                for (int k = 0; k < COLS; k++) if (!grid[r][k]) { full = false; break; }
                if (full) { lines++; continue; }
                for (int k = 0; k < COLS; k++) g2[dst][k] = grid[r][k];
                dst--;
            }
            int agg = 0, holes = 0, bump = 0, prev = -1;
            for (int k = 0; k < COLS; k++) {
                int h = 0;
                bool seen = false;
                for (int r = 0; r < ROWS; r++) {
                    if (g2[r][k]) { if (!seen) { h = ROWS - r; seen = true; } }
                    else if (seen) holes++;
                }
                agg += h;
                if (prev >= 0) bump += h > prev ? h - prev : prev - h;
                prev = h;
            }
            double score = -0.51 * agg + 0.76 * lines - 0.36 * holes - 0.18 * bump;
            opts.push_back({rot, x, score});
        }
    }
    if (opts.empty()) { targetRot_ = cur.rot; targetX_ = cur.x; return; }

    size_t best = 0;
    for (size_t i = 1; i < opts.size(); i++) if (opts[i].score > opts[best].score) best = i;
    size_t pick = best;
    if (std::uniform_real_distribution<double>(0, 1)(rng_) > skill_) pick = rng_() % opts.size();   // errou de proposito
    targetRot_ = opts[pick].rot;
    targetX_ = opts[pick].x;
}

void Bot::update(game::Game& g, int dtMs) {
    if (!g.active() || !g.has_piece()) return;
    if (g.pieces_spawned() != lastSpawn_) {
        lastSpawn_ = g.pieces_spawned();
        plan(g);
        acc_ = 0;
    }
    acc_ += dtMs;
    while (acc_ >= stepMs_) {
        acc_ -= stepMs_;
        const game::Piece& p = g.current();
        if (p.rot != targetRot_) { if (!g.rotate(1)) targetRot_ = p.rot; }
        else if (p.x != targetX_) { if (!g.move(p.x < targetX_ ? 1 : -1)) targetX_ = p.x; }
        else { g.hard_drop(); return; }
    }
}

}  // namespace bot
