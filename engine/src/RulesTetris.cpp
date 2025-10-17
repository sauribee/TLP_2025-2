#include "IGame.hpp"
#include "Engine.hpp"
#include <algorithm>
#include <array>
#include <memory>
#include <random>
#include <string>
#include <vector>

// ====== Utilidades de dibujo del tablero ======
static void draw_board_frame(GameContext& ctx, SDL_Color border, SDL_Color grid) {
    SDL_Renderer* r = ctx.eng.renderer();
    const int W = ctx.cols * ctx.cell_px, H = ctx.rows * ctx.cell_px;

    // grilla fina
    SDL_SetRenderDrawColor(r, grid.r, grid.g, grid.b, grid.a);
    for (int x = 1; x < ctx.cols; ++x)
        SDL_RenderDrawLine(r, x*ctx.cell_px, 0, x*ctx.cell_px, H);
    for (int y = 1; y < ctx.rows; ++y)
        SDL_RenderDrawLine(r, 0, y*ctx.cell_px, W, y*ctx.cell_px);

    // borde
    SDL_Rect frame{0,0,W,H};
    SDL_SetRenderDrawColor(r, border.r, border.g, border.b, border.a);
    SDL_RenderDrawRect(r, &frame);
}

// ====== Tetrominós 4×4 y rotación ======
using Rot = std::array<std::string,4>;   // 4 filas de largo 4 ('.' o '#')
using Shape = std::array<Rot,4>;         // 4 rotaciones

static Rot rotCW(const Rot& r) {
    Rot o;
    for (int y=0; y<4; ++y) {
        std::string row(4,'.');
        for (int x=0; x<4; ++x) row[x] = r[3-x][y];
        o[y] = row;
    }
    return o;
}

static std::array<Shape,7> make_shapes() {
    std::array<Shape,7> S;

    // I
    Rot I0{{"....","####","....","...."}};
    Shape I{ I0, rotCW(I0), rotCW(rotCW(I0)), rotCW(rotCW(rotCW(I0))) };
    // O
    Rot O0{{".##.",".##.","....","...."}};
    Shape O{ O0, O0, O0, O0 };
    // T
    Rot T0{{".#..","###.","....","...."}};
    Shape T{ T0, rotCW(T0), rotCW(rotCW(T0)), rotCW(rotCW(rotCW(T0))) };
    // L
    Rot L0{{"..#.","..#.",".##.","...."}};
    Shape L{ L0, rotCW(L0), rotCW(rotCW(L0)), rotCW(rotCW(rotCW(L0))) };
    // J
    Rot J0{{".#..",".#..",".##.","...."}};
    Shape J{ J0, rotCW(J0), rotCW(rotCW(J0)), rotCW(rotCW(rotCW(J0))) };
    // S
    Rot S0{{".##.","##..","....","...."}};
    Shape Sshape{ S0, rotCW(S0), rotCW(rotCW(S0)), rotCW(rotCW(rotCW(S0))) };
    // Z
    Rot Z0{{"##..",".##.","....","...."}};
    Shape Z{ Z0, rotCW(Z0), rotCW(rotCW(Z0)), rotCW(rotCW(rotCW(Z0))) };

    S[0]=I; S[1]=O; S[2]=T; S[3]=L; S[4]=J; S[5]=Sshape; S[6]=Z;
    return S;
}
static const auto SHAPES = make_shapes();

static SDL_Color color_of(int t) {
    switch (t) {
        case 1: return SDL_Color{  0,255,255,255}; // I
        case 2: return SDL_Color{255,255,  0,255}; // O
        case 3: return SDL_Color{128,  0,128,255}; // T
        case 4: return SDL_Color{255,165,  0,255}; // L
        case 5: return SDL_Color{  0,  0,255,255}; // J
        case 6: return SDL_Color{  0,255,  0,255}; // S
        case 7: return SDL_Color{255,  0,  0,255}; // Z
        default:return SDL_Color{200,200,200,255};
    }
}

struct Active {
    int type=1;    // 1..7
    int rot=0;     // 0..3
    int x=3, y=-1; // top-left de la máscara 4×4 (y puede ser -1 al aparecer)
};

class GameTetris : public IGame {
    int cols=10, rows=20, cell=24;
    std::vector<int> grid;   // rows*cols, 0 vacío o 1..7 tipo
    Active cur{};
    float acc=0.f, gravity=0.5f; // seg por caída de 1 celda
    bool game_over=false;
    std::mt19937 rng{1234567};

    // input repeat handling
    float left_timer=0.f, right_timer=0.f, rotate_timer=0.f, down_timer=0.f;
    bool left_prev=false, right_prev=false, rotate_prev=false, down_prev=false;
    const float move_initial_delay = 0.18f;
    const float move_repeat_interval = 0.06f;
    const float rotate_initial_delay = 0.20f;
    const float rotate_repeat_interval = 0.12f;
    const float down_repeat_interval = 0.05f;

    int idx(int x,int y) const { return y*cols + x; }
    bool in_bounds(int x,int y) const { return x>=0 && x<cols && y>=0 && y<rows; }

    bool cell_occupied(int x,int y) const {
        return in_bounds(x,y) ? (grid[idx(x,y)] != 0) : true; // fuera de bounds cuenta como colisión
    }

    bool collides(const Active& a) const {
        const auto& rot = SHAPES[a.type-1][a.rot];
        for (int dy=0; dy<4; ++dy) for (int dx=0; dx<4; ++dx) {
            if (rot[dy][dx] == '#') {
                int X = a.x + dx;
                int Y = a.y + dy;
                if (Y < 0) continue;           // por encima del tope visible no colisiona con grid
                if (X < 0 || X >= cols || Y >= rows) return true;
                if (cell_occupied(X,Y)) return true;
            }
        }
        return false;
    }

    void lock_piece(bool& topout) {
        topout = false;
        const auto& rot = SHAPES[cur.type-1][cur.rot];
        for (int dy=0; dy<4; ++dy) for (int dx=0; dx<4; ++dx) {
            if (rot[dy][dx] == '#') {
                int X = cur.x + dx;
                int Y = cur.y + dy;
                if (Y < 0) { topout = true; continue; }
                if (in_bounds(X,Y)) grid[idx(X,Y)] = cur.type;
            }
        }
    }

    int clear_lines() {
        int cleared=0;
        for (int y=rows-1; y>=0; --y) {
            bool full=true;
            for (int x=0; x<cols; ++x) if (grid[idx(x,y)]==0) { full=false; break; }
            if (full) {
                for (int yy=y; yy>0; --yy)
                    for (int x=0; x<cols; ++x)
                        grid[idx(x,yy)] = grid[idx(x,yy-1)];
                for (int x=0; x<cols; ++x) grid[idx(x,0)]=0;
                ++cleared;
                ++y; // re-evaluar esta fila
            }
        }
        return cleared;
    }

    void spawn() {
        std::uniform_int_distribution<int> dist(1,7);
        cur.type = dist(rng);
        cur.rot  = 0;
        cur.x    = cols/2 - 2; // centrado aprox. para máscaras 4×4
        cur.y    = -1;
        if (collides(cur)) game_over = true; // top-out al aparecer
    }

    void rotate(int dir) {
        Active test = cur;
        test.rot = (test.rot + (dir>0?1:3)) & 3;
        // Patadas laterales simples
        const int kicks[5] = {0,-1,1,-2,2};
        for (int k : kicks) {
            test.x = cur.x + k;
            if (!collides(test)) { cur = test; return; }
        }
    }

public:
    void init(GameContext& ctx) override {
        ctx.cols = cols; ctx.rows = rows; ctx.cell_px = cell;
        ctx.input = InputMap::defaults_tetris();
        grid.assign(rows*cols, 0);
        acc=0.f; ctx.score=0; game_over=false;

        // reset input repeat state
        left_timer = right_timer = rotate_timer = down_timer = 0.f;
        left_prev = right_prev = rotate_prev = down_prev = false;

        spawn();

    }

    void update(GameContext& ctx, float dt) override {
        if (game_over) {
            if (ctx.input.down(ctx.eng, "restart")) { init(ctx); }
            return;
        }

                // entrada lateral / soft drop / rotación (con repeat timers)
        Active test = cur;
        bool left  = ctx.input.down(ctx.eng, "left");
        bool right = ctx.input.down(ctx.eng, "right");
        bool rot   = ctx.input.down(ctx.eng, "rotate");
        bool down  = ctx.input.down(ctx.eng, "down");

        if (left)  { if (!left_prev) { test = cur; test.x--; if(!collides(test)) cur = test; left_timer = move_initial_delay; }
                     else { left_timer -= dt; if (left_timer <= 0.f) { test = cur; test.x--; if(!collides(test)) cur = test; left_timer = move_repeat_interval; } } }
        else { left_timer = 0.f; }
        left_prev = left;

        if (right) { if (!right_prev) { test = cur; test.x++; if(!collides(test)) cur = test; right_timer = move_initial_delay; }
                     else { right_timer -= dt; if (right_timer <= 0.f) { test = cur; test.x++; if(!collides(test)) cur = test; right_timer = move_repeat_interval; } } }
        else { right_timer = 0.f; }
        right_prev = right;

        if (rot)   { if (!rotate_prev) { rotate(+1); rotate_timer = rotate_initial_delay; }
                     else { rotate_timer -= dt; if (rotate_timer <= 0.f) { rotate(+1); rotate_timer = rotate_repeat_interval; } } }
        else { rotate_timer = 0.f; }
        rotate_prev = rot;

        if (down)  { if (!down_prev) { test = cur; test.y++; if(!collides(test)) { cur = test; ctx.score += 1; } down_timer = down_repeat_interval; }
                     else { down_timer -= dt; if (down_timer <= 0.f) { test = cur; test.y++; if(!collides(test)) { cur = test; ctx.score += 1; } down_timer = down_repeat_interval; } } }
        else { down_timer = 0.f; }
        down_prev = down;

        // // entrada lateral / soft drop / rotación
        // Active test = cur;
        // if (ctx.input.down(ctx.eng, "left"))  { test = cur; test.x--; if(!collides(test)) cur=test; }
        // if (ctx.input.down(ctx.eng, "right")) { test = cur; test.x++; if(!collides(test)) cur=test; }
        // if (ctx.input.down(ctx.eng, "rotate")) { rotate(+1); }
        // if (ctx.input.down(ctx.eng, "down"))  { test = cur; test.y++; if(!collides(test)) { cur=test; ctx.score += 1; } }

        // gravedad
        acc += dt;
        while (acc >= gravity) {
            acc -= gravity;
            test = cur; test.y++;
            if (collides(test)) {
                bool topout=false;
                lock_piece(topout);
                int c = clear_lines();
                if (c>0) ctx.score += 100*c;
                if (topout) { game_over = true; return; }
                spawn();
                if (game_over) return;
            } else {
                cur = test;
            }
        }
    }

    void render(GameContext& ctx) override {
        auto& eng = ctx.eng;
        eng.clear(18,18,18,255);

        // grilla + borde del tablero para referencia visual
        draw_board_frame(ctx, SDL_Color{255,200,40,255}, SDL_Color{55,55,55,255});

        // celdas fijas
        for (int y=0; y<rows; ++y) for (int x=0; x<cols; ++x) {
            int t = grid[idx(x,y)];
            if (t!=0) eng.draw_brick(x*cell, y*cell, cell, color_of(t));
        }

        // pieza activa (si no hay game over)
        if (!game_over) {
            const auto& rot = SHAPES[cur.type-1][cur.rot];
            for (int dy=0; dy<4; ++dy) for (int dx=0; dx<4; ++dx) {
                if (rot[dy][dx]=='#') {
                    int X = cur.x + dx;
                    int Y = cur.y + dy;
                    if (Y>=0 && X>=0 && X<cols) eng.draw_brick(X*cell, Y*cell, cell, color_of(cur.type));
                }
            }
        }

        if (game_over) {
            eng.draw_text(8, 8, "TETRIS — GAME OVER. R reinicia. ESC sale.",
                          SDL_Color{255,120,120,255});
        } else {
            eng.draw_text(8, 8, "TETRIS — ←/→ mueve, ↓ baja(+1), ↑ rota, ESC sale, R reinicia.",
                          SDL_Color{255,255,255,255});
        }
        eng.draw_text(8, 32, "Score: " + std::to_string(ctx.score), SDL_Color{255,255,255,255});
        eng.present();
    }
};

std::unique_ptr<IGame> make_tetris() { return std::make_unique<GameTetris>(); }
