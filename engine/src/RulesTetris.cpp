#include "IGame.hpp"
#include "Engine.hpp"
#include <algorithm>
#include <array>
#include <memory>
#include <random>
#include <string>
#include <vector>
#include <deque>
#include <cmath> // round

// Devuelve true sólo cuando la tecla pasa de "no pulsada" a "pulsada".
static bool pressed_once(const InputMap& in, const Engine& eng, const char* action, bool& latch) {
    bool d = in.down(eng, action);
    bool just = (d && !latch);
    latch = d;
    return just;
}

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
using Rot   = std::array<std::string,4>; // 4 filas de largo 4 ('.' o '#')
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

// Partículas para "line clear"
struct Particle {
    float x,y, vx,vy, life; // life: 0..1
};

class GameTetris : public IGame {
    int cols=10, rows=20;
public:
    int cell=24;
private:
    std::vector<int> grid;   // rows*cols, 0 vacío o 1..7 tipo
    Active cur{};
    float acc=0.f;
    float baseGravity_=0.55f; // gravedad base (s por celda)
    float gravity=baseGravity_;
    bool game_over=false;
    std::mt19937 rng{1234567};

    // Progresión
    int level_ = 1;
    int linesTotal_ = 0;

    // input repeat handling (flechas/↑/↓)
    float left_timer=0.f, right_timer=0.f, rotate_timer=0.f, down_timer=0.f;
    bool left_prev=false, right_prev=false, rotate_prev=false, down_prev=false;
    const float move_initial_delay = 0.18f;
    const float move_repeat_interval = 0.06f;
    const float rotate_initial_delay = 0.20f;
    const float rotate_repeat_interval = 0.12f;
    const float down_repeat_interval = 0.05f;

    // UI
    bool paused_ = false;
    bool showHelp_ = false;
    bool latchPause_ = false;
    bool latchHelp_ = false;

    // Pulsos
    bool latchHardDrop_ = false;
    bool latchHold_     = false;
    bool latchRotCW_    = false;
    bool latchRotCCW_   = false;

    // Hold
    int  holdType_   = 0;   // 0 = vacío; 1..7 = tipo guardado
    bool holdLocked_ = false; // no se puede volver a usar hold hasta bloquear

    // Partículas
    std::vector<Particle> particles_;

    // Next queue (x3)
    std::array<int,3> next_{};

    // Lock-delay
    bool grounded_ = false;
    float lockTimer_ = 0.f;
    const float lockDelay_ = 0.35f; // segundos

    int idx(int x,int y) const { return y*cols + x; }
    bool in_bounds(int x,int y) const { return x>=0 && x<cols && y>=0 && y<rows; }

    bool cell_occupied(int x,int y) const {
        return in_bounds(x,y) ? (grid[idx(x,y)] != 0) : true; // fuera de bounds = colisión
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

    // y donde quedaría el "ghost" si cae recto
    int computeGhostY(const Active& a) const {
        Active t = a;
        while (true) {
            t.y++;
            if (collides(t)) return t.y - 1;
        }
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
        holdLocked_ = false; // al bloquear, se libera el hold
        grounded_ = false;
        lockTimer_ = 0.f;
    }

    void emit_line_particles(int y) {
        for (int x=0; x<cols; ++x) {
            int n = 2 + (rng()%2);
            for (int i=0;i<n;++i){
                Particle p;
                p.x = x*cell + cell*0.5f;
                p.y = y*cell + cell*0.5f;
                p.vx = ((rng()%200)-100)/100.0f * 120.f;
                p.vy = -((rng()%100)/100.0f) * 150.f;
                p.life = 1.f;
                particles_.push_back(p);
            }
        }
    }

    int clear_lines() {
        int cleared=0;
        for (int y=rows-1; y>=0; --y) {
            bool full=true;
            for (int x=0; x<cols; ++x) if (grid[idx(x,y)]==0) { full=false; break; }
            if (full) {
                emit_line_particles(y);
                for (int yy=y; yy>0; --yy)
                    for (int x=0; x<cols; ++x)
                        grid[idx(x,yy)] = grid[idx(x,yy-1)];
                for (int x=0; x<cols; ++x) grid[idx(x,0)]=0;
                ++cleared;
                ++y; // re-evaluar esta fila
            }
        }
        if (cleared>0) {
            linesTotal_ += cleared;
            int newLevel = 1 + (linesTotal_ / 10);
            if (newLevel != level_) {
                level_ = newLevel;
                gravity = std::max(0.08f, baseGravity_ / (1.f + 0.15f*(level_-1)));
            }
        }
        return cleared;
    }

    int rand_piece() {
        std::uniform_int_distribution<int> dist(1,7);
        return dist(rng);
    }

    void spawn() {
        // usa cola Next
        cur.type = next_[0];
        next_[0] = next_[1];
        next_[1] = next_[2];
        next_[2] = rand_piece();

        cur.rot  = 0;
        cur.x    = cols/2 - 2; // centrado aprox. para máscaras 4×4
        cur.y    = -1;
        if (collides(cur)) game_over = true; // top-out al aparecer
        grounded_ = false;
        lockTimer_ = 0.f;
    }

    bool rotate_try(int dir) {
        Active test = cur;
        test.rot = (test.rot + (dir>0?1:3)) & 3;
        const int kicks[5] = {0,-1,1,-2,2};
        for (int k : kicks) {
            test.x = cur.x + k;
            if (!collides(test)) { cur = test; return true; }
        }
        return false;
    }

    void hold_piece() {
        if (holdLocked_) return;
        if (holdType_ == 0) {
            holdType_ = cur.type;
            spawn();
        } else {
            std::swap(holdType_, cur.type);
            cur.rot = 0;
            cur.x = cols/2 - 2;
            cur.y = -1;
            if (collides(cur)) game_over = true;
        }
        holdLocked_ = true; // sólo una vez por pieza caída
        grounded_ = false;
        lockTimer_ = 0.f;
    }

    // Mini render de "next"/"hold" (4x4)
    void draw_mini(GameContext& ctx, int type, int ox, int oy, int mini) {
        SDL_Renderer* R = ctx.eng.renderer();
        SDL_Rect box{ox-2, oy-2, 4*mini+4, 4*mini+4};
        SDL_SetRenderDrawColor(R, 255,255,255,30); SDL_RenderDrawRect(R, &box);
        if (type<=0) return;
        const auto& r0 = SHAPES[type-1][0];
        SDL_Color col = color_of(type);
        for (int y=0;y<4;++y) for (int x=0;x<4;++x) {
            if (r0[y][x]=='#') {
                SDL_Rect rc{ ox + x*mini, oy + y*mini, mini, mini };
                SDL_SetRenderDrawColor(R, col.r, col.g, col.b, 255);
                SDL_RenderFillRect(R, &rc);
            }
        }
    }

public:
    void init(GameContext& ctx) override {
        ctx.cols = cols; ctx.rows = rows; ctx.cell_px = cell;
        ctx.input = InputMap::defaults_tetris();

        grid.assign(rows*cols, 0);
        particles_.clear();
        holdType_ = 0; holdLocked_ = false;

        game_over=false; acc=0.f;
        level_=1; linesTotal_=0; gravity = baseGravity_;
        grounded_ = false; lockTimer_ = 0.f;

        next_[0]=rand_piece(); next_[1]=rand_piece(); next_[2]=rand_piece();
        spawn();

        // reset input repeat state
        left_timer = right_timer = rotate_timer = down_timer = 0.f;
        left_prev = right_prev = rotate_prev = down_prev = false;
        latchHardDrop_ = latchHold_ = latchRotCW_ = latchRotCCW_ = false;
        paused_ = false; showHelp_ = false; latchPause_ = latchHelp_ = false;
    }

    void update(GameContext& ctx, float dt) override {
        if (game_over) {
            if (ctx.input.down(ctx.eng, "restart")) { init(ctx); }
            return;
        }

        // Toggle pausa y overlay de ayuda
        if (pressed_once(ctx.input, ctx.eng, "pause", latchPause_))  paused_  = !paused_;
        if (pressed_once(ctx.input, ctx.eng, "help",  latchHelp_ ))  showHelp_ = !showHelp_;
        if (paused_) {
            if (ctx.input.down(ctx.eng, "restart")) { init(ctx); }
            return;
        }

        bool movedOrRotated = false;

        // ===== Entrada lateral / rotación / soft drop (flechas, con repeat timers)
        Active test = cur;
        bool left  = ctx.input.down(ctx.eng, "left");
        bool right = ctx.input.down(ctx.eng, "right");
        bool rotUp = ctx.input.down(ctx.eng, "rotate"); // ↑
        bool down  = ctx.input.down(ctx.eng, "down") || ctx.input.down(ctx.eng, "soft_drop"); // ↓ o 'S'

        if (left)  {
            if (!left_prev) { test = cur; test.x--; if(!collides(test)) { cur = test; movedOrRotated=true; } left_timer = move_initial_delay; }
            else { left_timer -= dt; if (left_timer <= 0.f) { test = cur; test.x--; if(!collides(test)) { cur = test; movedOrRotated=true; } left_timer = move_repeat_interval; } }
        } else left_timer = 0.f;
        left_prev = left;

        if (right) {
            if (!right_prev) { test = cur; test.x++; if(!collides(test)) { cur = test; movedOrRotated=true; } right_timer = move_initial_delay; }
            else { right_timer -= dt; if (right_timer <= 0.f) { test = cur; test.x++; if(!collides(test)) { cur = test; movedOrRotated=true; } right_timer = move_repeat_interval; } }
        } else right_timer = 0.f;
        right_prev = right;

        if (rotUp) {
            if (!rotate_prev) { if (rotate_try(+1)) movedOrRotated=true; rotate_timer = rotate_initial_delay; }
            else { rotate_timer -= dt; if (rotate_timer <= 0.f) { if (rotate_try(+1)) movedOrRotated=true; rotate_timer = rotate_repeat_interval; } }
        } else rotate_timer = 0.f;
        rotate_prev = rotUp;

        if (down)  {
            if (!down_prev) { test = cur; test.y++; if(!collides(test)) { cur = test; ctx.score += 1; movedOrRotated=true; } down_timer = down_repeat_interval; }
            else { down_timer -= dt; if (down_timer <= 0.f) { test = cur; test.y++; if(!collides(test)) { cur = test; ctx.score += 1; movedOrRotated=true; } down_timer = down_repeat_interval; } }
        } else down_timer = 0.f;
        down_prev = down;

        // ===== Acciones "pulse": rotaciones Z/X, hold A, hard drop SPACE
        if (pressed_once(ctx.input, ctx.eng, "rotate_ccw", latchRotCCW_)) { if (rotate_try(-1)) movedOrRotated=true; }
        if (pressed_once(ctx.input, ctx.eng, "rotate_cw",  latchRotCW_ )) { if (rotate_try(+1)) movedOrRotated=true; }
        if (pressed_once(ctx.input, ctx.eng, "hold",       latchHold_  )) { hold_piece(); movedOrRotated=true; }

        if (pressed_once(ctx.input, ctx.eng, "hard_drop",  latchHardDrop_)) {
            int gy = computeGhostY(cur);
            int dropped = std::max(0, gy - cur.y);
            cur.y = gy;
            bool topout=false;
            lock_piece(topout);
            int c = clear_lines();
            if (c>0) ctx.score += 100*c;
            ctx.score += 2 * dropped; // bonificación por hard drop
            if (topout) { game_over = true; return; }
            spawn();
            acc = 0.f;
            return;
        }

        // Si tocamos el piso y nos movemos/rotamos, reinicia lock-delay
        if (grounded_ && movedOrRotated) lockTimer_ = 0.f;

        // ===== Gravedad + Lock-delay
        acc += dt;
        while (acc >= gravity) {
            acc -= gravity;
            test = cur; test.y++;
            if (collides(test)) {
                grounded_ = true;
                lockTimer_ += gravity;
                if (lockTimer_ >= lockDelay_) {
                    bool topout=false;
                    lock_piece(topout);
                    int c = clear_lines();
                    if (c>0) ctx.score += 100*c;
                    if (topout) { game_over = true; return; }
                    spawn();
                    if (game_over) return;
                }
            } else {
                grounded_ = false;
                lockTimer_ = 0.f;
                cur = test;
            }
        }

        // ===== Actualizar partículas
        if (!particles_.empty()) {
            for (auto& p : particles_) {
                p.x += p.vx * dt;
                p.y += p.vy * dt;
                p.vy += 300.f * dt; // gravedad
                p.life -= 1.5f * dt;
            }
            particles_.erase(std::remove_if(particles_.begin(), particles_.end(),
                               [](const Particle& p){ return p.life <= 0.f; }),
                             particles_.end());
        }
    }

void render(GameContext& ctx) override {
    auto& eng = ctx.eng;
    SDL_Renderer* R = eng.renderer();
    eng.clear(18,18,18,255);

    // Dimensiones de ventana
    int winW=0, winH=0; 
    SDL_GetRendererOutputSize(R, &winW, &winH);

    // --- Tablero centrado en viewport ---
    const int boardW = ctx.cols * ctx.cell_px;
    const int boardH = ctx.rows * ctx.cell_px;
    int originX = std::max(0, (winW - boardW)/2);
    int originY = std::max(0, (winH - boardH)/2);
    SDL_Rect vp{ originX, originY, boardW, boardH };
    SDL_RenderSetViewport(R, &vp);

    SDL_SetRenderDrawBlendMode(R, SDL_BLENDMODE_BLEND);
    draw_board_frame(ctx, SDL_Color{255,200,40,255}, SDL_Color{55,55,55,255});

    // celdas fijas
    for (int y=0; y<rows; ++y) for (int x=0; x<cols; ++x) {
        int t = grid[idx(x,y)];
        if (t!=0) eng.draw_brick(x*ctx.cell_px, y*ctx.cell_px, ctx.cell_px, color_of(t));
    }

    // Ghost
    if (!game_over) {
        Active ghost = cur; 
        ghost.y = computeGhostY(cur);
        const auto& rot = SHAPES[ghost.type-1][ghost.rot];
        for (int dy=0; dy<4; ++dy) for (int dx=0; dx<4; ++dx) {
            if (rot[dy][dx]=='#') {
                int X = ghost.x + dx, Y = ghost.y + dy;
                if (Y>=0 && X>=0 && X<cols) {
                    SDL_Rect rc{ X*ctx.cell_px, Y*ctx.cell_px, ctx.cell_px, ctx.cell_px };
                    SDL_SetRenderDrawColor(R, 200,200,200, 70);
                    SDL_RenderFillRect(R, &rc);
                }
            }
        }
    }

    // Activa
    if (!game_over) {
        const auto& rot = SHAPES[cur.type-1][cur.rot];
        for (int dy=0; dy<4; ++dy) for (int dx=0; dx<4; ++dx) {
            if (rot[dy][dx]=='#') {
                int X = cur.x + dx, Y = cur.y + dy;
                if (Y>=0 && X>=0 && X<cols)
                    eng.draw_brick(X*ctx.cell_px, Y*ctx.cell_px, ctx.cell_px, color_of(cur.type));
            }
        }
    }

    // Partículas (coordenadas de tablero)
    for (auto& p : particles_) {
        int alpha = (int)std::round(200.f * std::max(0.f, p.life));
        SDL_SetRenderDrawColor(R, 255,220,60, alpha);
        SDL_Rect rc{ int(p.x)-2, int(p.y)-2, 4, 4 };
        SDL_RenderFillRect(R, &rc);
    }

    // === Salimos del viewport: todo lo que sigue es a nivel de ventana ===
    SDL_RenderSetViewport(R, nullptr);

    // --------- HUD superior (ventana completa) ----------
    {
        const int barH = 24; // banda superior delgada
        SDL_Rect bar{0,0,winW,barH};
        SDL_SetRenderDrawColor(R, 0,0,0,150);
        SDL_RenderFillRect(R, &bar);

        // Score (izquierda)
        eng.draw_text(8, 4, "Score: " + std::to_string(ctx.score), SDL_Color{255,255,255,255});
        // Hints (centrados en ventana)
        eng.draw_text(winW/2 - 90, 4, "H:Help  P:Pause  ESC", SDL_Color{220,220,220,255});

        // Mini UI (Hold/Next) debajo de la barra, en coords de ventana
        const int mini = 8;
        const int pad  = 6;
        const int topY = barH + 4;

        // Hold (esquina izq)
        draw_mini(ctx, holdType_, pad, topY, mini);

        // Next vertical (esquina der)
        int nx = winW - pad - 4*mini;
        int ny = topY;
        draw_mini(ctx, next_[0], nx, ny, mini);            ny += 4*mini + 4;
        draw_mini(ctx, next_[1], nx, ny, mini);            ny += 4*mini + 4;
        draw_mini(ctx, next_[2], nx, ny, mini);
    }

    // ---------- Overlays (Help / Pausa / Game Over) en ventana ----------
    if (showHelp_) {
        // Fondo translúcido full-window
        SDL_SetRenderDrawColor(R, 0,0,0,190);
        SDL_Rect shade{0,0,winW,winH};
        SDL_RenderFillRect(R, &shade);

        // Texto pequeño (80%) con escala del renderer
        auto draw_small = [&](int x, int y, const std::string& s, SDL_Color c){
            const float sX = 0.80f, sY = 0.80f;
            SDL_RenderSetScale(R, sX, sY);
            eng.draw_text(int(x / sX), int(y / sY), s, c);
            SDL_RenderSetScale(R, 1.f, 1.f);
        };

        // Panel centrado
        const int panelW = std::min(520, winW - 40);
        const int panelH = 150;
        const int px = (winW - panelW)/2;
        const int py = (winH - panelH)/2;
        SDL_SetRenderDrawColor(R, 20,20,20,230);
        SDL_Rect panel{px, py, panelW, panelH};
        SDL_RenderFillRect(R, &panel);
        SDL_SetRenderDrawColor(R, 255,200,40,220);
        SDL_RenderDrawRect(R, &panel);

        int y = py + 12;
        draw_small(px + 16, y, "TETRIS — Quick Help", SDL_Color{255,255,255,255}); y += 20;
        draw_small(px + 16, y, "Score: " + std::to_string(ctx.score) +
                               "   Lvl: " + std::to_string(level_) +
                               "   Lines: " + std::to_string(linesTotal_), SDL_Color{230,230,230,255}); y += 20;
        draw_small(px + 16, y, "Move:  ← / →     Soft:  ↓ or S     Hard:  Space", SDL_Color{220,220,220,255}); y += 18;
        draw_small(px + 16, y, "Rotate:  ↑ or X (CW) ,  Z (CCW)     Hold:  A",   SDL_Color{220,220,220,255}); y += 18;
        draw_small(px + 16, y, "Pause:  P     Reset:  R     Close:  ESC",         SDL_Color{220,220,220,255});
    }

    if (paused_ && !game_over) {
        SDL_SetRenderDrawColor(R, 0,0,0,140);
        SDL_Rect full{0,0,winW,winH}; SDL_RenderFillRect(R, &full);
        eng.draw_text(winW/2 - 28, winH/2 - 8, "PAUSED", SDL_Color{255,255,255,255});
    }

    if (game_over) {
        SDL_SetRenderDrawColor(R, 0,0,0,160);
        SDL_Rect full{0,0,winW,winH}; SDL_RenderFillRect(R, &full);
        eng.draw_text(winW/2 - 90, winH/2 - 10, "GAME OVER — R to restart", SDL_Color{255,120,120,255});
    }

    eng.present();
}

};

std::unique_ptr<IGame> make_tetris() { return std::make_unique<GameTetris>(); }
