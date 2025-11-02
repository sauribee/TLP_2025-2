#include "IGame.hpp"
#include "Engine.hpp"
#include <deque>
#include <random>
#include <memory>
#include <string>
#include <cmath>    // sinf, fmodf, fabsf

// --------------- helpers input -----------------
static bool pressed_once(const InputMap& in, const Engine& eng,
                         const char* action, bool& latch) {
    bool d = in.down(eng, action);
    bool just = (d && !latch);
    latch = d;
    return just;
}

// --------------- helpers color/board ------------
struct RGB { Uint8 r,g,b; };
static RGB hsv2rgb(float h, float s, float v) {
    float c = v * s;
    float x = c * (1 - std::fabsf(std::fmodf(h/60.0f, 2.0f) - 1));
    float m = v - c;
    float r=0,g=0,b=0;
    int hi = int(h/60.0f) % 6;
    switch (hi) {
        case 0: r=c; g=x; b=0; break; case 1: r=x; g=c; b=0; break;
        case 2: r=0; g=c; b=x; break; case 3: r=0; g=x; b=c; break;
        case 4: r=x; g=0; b=c; break; default: r=c; g=0; b=x; break;
    }
    return RGB{ Uint8((r+m)*255), Uint8((g+m)*255), Uint8((b+m)*255) };
}

static void draw_board_frame(GameContext& ctx, SDL_Color border, SDL_Color grid) {
    SDL_Renderer* r = ctx.eng.renderer();
    const int W = ctx.cols * ctx.cell_px, H = ctx.rows * ctx.cell_px;

    SDL_SetRenderDrawColor(r, grid.r, grid.g, grid.b, grid.a);
    for (int x = 1; x < ctx.cols; ++x)
        SDL_RenderDrawLine(r, x*ctx.cell_px, 0, x*ctx.cell_px, H);
    for (int y = 1; y < ctx.rows; ++y)
        SDL_RenderDrawLine(r, 0, y*ctx.cell_px, W, y*ctx.cell_px);

    SDL_Rect frame{0,0,W,H};
    SDL_SetRenderDrawColor(r, border.r, border.g, border.b, border.a);
    SDL_RenderDrawRect(r, &frame);
}

static void draw_eyes(SDL_Renderer* R, const SDL_Rect& head, int dirx, int diry) {
    int offx = dirx * (head.w/6), offy = diry * (head.h/6);
    SDL_Rect eyeL{ head.x + head.w/4 + offx, head.y + head.h/3 + offy, head.w/8, head.h/8 };
    SDL_Rect eyeR{ head.x + head.w/2 + offx, head.y + head.h/3 + offy, head.w/8, head.h/8 };
    SDL_SetRenderDrawColor(R, 255,255,255,255); SDL_RenderFillRect(R, &eyeL); SDL_RenderFillRect(R, &eyeR);
    SDL_Rect pL{ eyeL.x+eyeL.w/3, eyeL.y+eyeL.h/3, eyeL.w/3, eyeL.h/3 };
    SDL_Rect pR{ eyeR.x+eyeR.w/3, eyeR.y+eyeR.h/3, eyeR.w/3, eyeR.h/3 };
    SDL_SetRenderDrawColor(R, 20,20,20,255); SDL_RenderFillRect(R, &pL); SDL_RenderFillRect(R, &pR);
}

static void draw_head(SDL_Renderer* R, const SDL_Rect& rc, bool arrow, int dirx, int diry, SDL_Color base, SDL_Color outline) {
    SDL_SetRenderDrawColor(R, base.r, base.g, base.b, base.a);
    SDL_RenderFillRect(R, &rc);
    if (arrow) {
        SDL_SetRenderDrawColor(R, Uint8(std::min(255, int(base.r)+30)),
                                  Uint8(std::min(255, int(base.g)+30)),
                                  Uint8(std::min(255, int(base.b)+30)), base.a);
        SDL_Rect tip = rc;
        if (dirx == 1) { tip.x = rc.x + (rc.w*3)/4; tip.w = rc.w/4; tip.y += rc.h/4; tip.h = rc.h/2; }
        if (dirx == -1){ tip.w = rc.w/4; tip.y += rc.h/4; tip.h = rc.h/2; }
        if (diry == 1) { tip.y = rc.y + (rc.h*3)/4; tip.h = rc.h/4; tip.x += rc.w/4; tip.w = rc.w/2; }
        if (diry == -1){ tip.h = rc.h/4; tip.x += rc.w/4; tip.w = rc.w/2; }
        SDL_RenderFillRect(R, &tip);
    }
    SDL_SetRenderDrawColor(R, outline.r, outline.g, outline.b, outline.a);
    SDL_RenderDrawRect(R, &rc);
    draw_eyes(R, rc, dirx, diry);
}

// ---------------------------------------------------------------

class GameSnake : public IGame {
    float acc = 0.f, step = 0.12f; // velocidad base
    int dirx = 1, diry = 0;        // derecha
    std::deque<SDL_Point> snake;   // cabeza = front
    SDL_Point fruit{5,5};
    std::mt19937 rng{12345};
    bool game_over = false;
    bool wrap_edges = false;       // false = muros

    // UX
    float timeSec_ = 0.f;
    float rainbowPhase_ = 0.f;
    bool headArrow_ = true;
    int paletteMode_ = 2;          // 0=classic, 1=neon, 2=rainbow
    bool latchToggleHead_ = false;
    bool latchTogglePalette_ = false;
    bool turbo_ = false;

    // Mejora #1: Fruta dorada (temporal)
    bool goldActive_ = false;
    SDL_Point goldPos_{0,0};
    float goldTimer_ = 0.f;  // segundos

    // Mejora #2: Combo/multiplicador
    int combo_ = 1;
    float comboTimer_ = 0.f; // si comes antes de 2.5s, sube el combo
    const float comboWindow_ = 2.5f;

    // Mejora #3: Portales A<->B (fijos pero seguros)
    bool portalsOn_ = true;
    SDL_Point portalA_{2,2}, portalB_{17,12};

    int wrap(int v, int m) { return (v % m + m) % m; }
    bool out_of_bounds(int x, int y, int cols, int rows) {
        return x < 0 || y < 0 || x >= cols || y >= rows;
    }

    bool occupies(int x, int y) const {
        for (auto& p : snake) if (p.x==x && p.y==y) return true;
        return false;
    }

    SDL_Point random_free_cell(GameContext& ctx) {
        std::uniform_int_distribution<int> cx(0, ctx.cols-1), cy(0, ctx.rows-1);
        SDL_Point p;
        do { p = SDL_Point{cx(rng), cy(rng)}; } while (occupies(p.x,p.y) || (portalsOn_ && ((p.x==portalA_.x&&p.y==portalA_.y)||(p.x==portalB_.x&&p.y==portalB_.y))));
        return p;
    }

    void reset(GameContext& ctx) {
        ctx.score = 0;
        snake.clear();
        snake.push_front(SDL_Point{3,7});
        snake.push_back(SDL_Point{2,7});
        snake.push_back(SDL_Point{1,7});
        dirx = 1; diry = 0;
        fruit = random_free_cell(ctx);
        acc = 0.f; game_over = false;
        timeSec_ = 0.f; rainbowPhase_ = 0.f; turbo_ = false;

        goldActive_ = false; goldTimer_ = 0.f;
        combo_ = 1; comboTimer_ = 0.f;

        // ajusta portales si caen sobre la serpiente
        if (occupies(portalA_.x, portalA_.y)) portalA_ = SDL_Point{4,2};
        if (occupies(portalB_.x, portalB_.y)) portalB_ = SDL_Point{15,12};
    }

    SDL_Color segment_color(size_t idx, size_t n) const {
        if (paletteMode_ == 0) { // classic: verdes
            return (idx==0) ? SDL_Color{70,220,140,255} : SDL_Color{80,200,120,255};
        }
        if (paletteMode_ == 1) { // neon: ciclo CMY
            switch (idx % 3) {
                case 0: return (idx==0) ? SDL_Color{ 80,255,255,255} : SDL_Color{ 60,220,220,255};
                case 1: return (idx==0) ? SDL_Color{255, 80,255,255} : SDL_Color{220, 60,220,255};
                default:return (idx==0) ? SDL_Color{255,255, 80,255} : SDL_Color{220,220, 60,255};
            }
        }
        float t = (n<=1) ? 0.f : float(idx)/float(n-1);
        float hue = std::fmod(360.f * (t + rainbowPhase_), 360.f);
        RGB c = hsv2rgb(hue, 0.85f, 0.95f);
        return SDL_Color{c.r, c.g, c.b, 255};
    }

    void maybe_spawn_gold(GameContext& ctx) {
        if (goldActive_) return;
        // 1/5 de probabilidad cada vez que comes
        if ((rng()%5)==0) {
            goldActive_ = true;
            goldTimer_ = 8.0f;
            goldPos_ = random_free_cell(ctx);
        }
    }

    void try_portal(int& nx, int& ny) {
        if (!portalsOn_) return;
        if (nx==portalA_.x && ny==portalA_.y) { nx = portalB_.x; ny = portalB_.y; }
        else if (nx==portalB_.x && ny==portalB_.y) { nx = portalA_.x; ny = portalA_.y; }
    }

public:
    void init(GameContext& ctx) override {
        ctx.cols = 20; ctx.rows = 15; ctx.cell_px = 24;
        ctx.input = InputMap::defaults_snake();
        reset(ctx);
    }

    void update(GameContext& ctx, float dt) override {
        timeSec_ += dt;
        rainbowPhase_ += dt * 0.10f; if (rainbowPhase_ >= 1.f) rainbowPhase_ -= 1.f;

        if (game_over) {
            if (ctx.input.down(ctx.eng, "restart")) reset(ctx);
            return;
        }

        // toggles visuales
        if (pressed_once(ctx.input, ctx.eng, "toggle_head", latchToggleHead_)) headArrow_ = !headArrow_;
        if (pressed_once(ctx.input, ctx.eng, "toggle_palette", latchTogglePalette_)) paletteMode_ = (paletteMode_ + 1) % 3;
        turbo_ = ctx.input.down(ctx.eng, "turbo");

        // entrada (evita reversa directa)
        if (ctx.input.down(ctx.eng, "left")  && dirx!=1)  { dirx=-1; diry=0; }
        if (ctx.input.down(ctx.eng, "right") && dirx!=-1) { dirx=1; diry=0; }
        if (ctx.input.down(ctx.eng, "up")    && diry!=1)  { dirx=0; diry=-1; }
        if (ctx.input.down(ctx.eng, "down")  && diry!=-1) { dirx=0; diry=1; }

        // timers
        if (comboTimer_ > 0.f) comboTimer_ -= dt;
        if (goldActive_) {
            goldTimer_ -= dt;
            if (goldTimer_ <= 0.f) goldActive_ = false;
        }

        // tick
        float curStep = turbo_ ? (step * 0.5f) : step;
        acc += dt;
        while (acc >= curStep) {
            acc -= curStep;
            SDL_Point head = snake.front();
            int nx = head.x + dirx, ny = head.y + diry;

            if (wrap_edges) {
                nx = wrap(nx, ctx.cols);
                ny = wrap(ny, ctx.rows);
            } else if (out_of_bounds(nx, ny, ctx.cols, ctx.rows)) {
                game_over = true; return;
            }

            // teletransporte por portales
            try_portal(nx, ny);

            // auto-colisión ⇒ GAME OVER
            for (auto& p : snake) if (p.x==nx && p.y==ny) { game_over = true; return; }

            bool grow = (nx == fruit.x && ny == fruit.y);
            bool gotGold = (goldActive_ && nx==goldPos_.x && ny==goldPos_.y);

            snake.push_front(SDL_Point{nx, ny});
            if (!grow && !gotGold) snake.pop_back();

            if (grow || gotGold) {
                // combo window
                combo_ = (comboTimer_ > 0.f) ? (combo_ + 1) : 1;
                comboTimer_ = comboWindow_;

                int base = grow ? 5 : 10;          // normal 5, dorada 10
                ctx.score += base * combo_;
                if (gotGold) { goldActive_ = false; } // consumir dorada
                // reubicar fruta normal + quizá dorada
                fruit = random_free_cell(ctx);
                maybe_spawn_gold(ctx);
            }
        }
    }

    void render(GameContext& ctx) override {
        auto& eng = ctx.eng;
        SDL_Renderer* R = eng.renderer();
        eng.clear(8,8,8,255);

        // --- Centrar tablero ---
        const int W = ctx.cols * ctx.cell_px, H = ctx.rows * ctx.cell_px;
        int winW=0, winH=0; SDL_GetRendererOutputSize(R, &winW, &winH);
        int originX = std::max(0, (winW - W)/2);
        int originY = std::max(0, (winH - H)/2);
        SDL_Rect vp{ originX, originY, W, H };
        SDL_RenderSetViewport(R, &vp);
        SDL_SetRenderDrawBlendMode(R, SDL_BLENDMODE_BLEND);

        draw_board_frame(ctx, SDL_Color{40,160,255,255}, SDL_Color{60,60,60,255});

        // fruta normal
        int s = ctx.cell_px;
        {
            SDL_Color c{255,80,80,255};
            eng.draw_brick(fruit.x*s, fruit.y*s, s, c);
        }
        // fruta dorada (pulso)
        if (goldActive_) {
            float pulse = 0.12f * (0.5f * std::sinf(timeSec_ * 2.0f * 3.1415926f * 1.2f) + 0.5f);
            int inflate = int(pulse * float(s));
            SDL_Rect blob{ goldPos_.x*s - inflate/2, goldPos_.y*s - inflate/2, s + inflate, s + inflate };
            SDL_SetRenderDrawColor(R, 255,215,0, 255);
            SDL_RenderFillRect(R, &blob);
        }

        // portales
        if (portalsOn_) {
            SDL_SetRenderDrawColor(R, 120,120,255,150);
            SDL_Rect a{ portalA_.x*s+2, portalA_.y*s+2, s-4, s-4 };
            SDL_Rect b{ portalB_.x*s+2, portalB_.y*s+2, s-4, s-4 };
            SDL_RenderDrawRect(R, &a); SDL_RenderDrawRect(R, &b);
        }

        // serpiente
        for (size_t i=0;i<snake.size();++i) {
            auto p = snake[i];
            SDL_Color c = segment_color(i, snake.size());

            if (i == 0) {
                SDL_Rect head{ p.x*s, p.y*s, s, s };
                draw_head(R, head, headArrow_, dirx, diry, c, SDL_Color{30,30,30,255});
            } else {
                int pad = 2;
                SDL_Rect inner{ p.x*s + pad, p.y*s + pad, s - 2*pad, s - 2*pad };
                SDL_SetRenderDrawColor(R, c.r, c.g, c.b, c.a);
                SDL_RenderFillRect(R, &inner);
            }
        }

        // HUD barra (Score, combo, timers)
        {
            const int barH = 28;
            SDL_Rect bar{0,0,W,barH};
            SDL_SetRenderDrawColor(R, 0,0,0,160); SDL_RenderFillRect(R, &bar);
            SDL_SetRenderDrawColor(R, 40,160,255,240); SDL_RenderDrawLine(R, 0, barH, W, barH);
            std::string hud = "Score: " + std::to_string(ctx.score);
            if (combo_>1) hud += "   Combo x" + std::to_string(combo_);
            if (goldActive_) hud += "   Gold " + std::to_string(int(std::ceil(goldTimer_))) + "s";
            if (turbo_) hud += "   (TURBO)";
            eng.draw_text(6, 6, hud, SDL_Color{255,255,255,255});
        }

        if (game_over) {
            SDL_SetRenderDrawColor(R, 0,0,0,160);
            SDL_Rect full{0,0,W,H}; SDL_RenderFillRect(R, &full);
            eng.draw_text(W/2 - 90, H/2 - 10, "GAME OVER — R to restart", SDL_Color{255,120,120,255});
        }

        SDL_RenderSetViewport(R, nullptr);
        eng.present();
    }
};

std::unique_ptr<IGame> make_snake() { return std::make_unique<GameSnake>(); }
