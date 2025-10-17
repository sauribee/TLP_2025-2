#include "IGame.hpp"
#include "Engine.hpp"
#include <deque>
#include <random>
#include <memory>
#include <string>

// Dibujar borde y grilla del tablero (0,0) a (cols*cell, rows*cell)
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

class GameSnake : public IGame {
    float acc = 0.f, step = 0.12f; // velocidad de tick
    int dirx = 1, diry = 0;        // derecha
    std::deque<SDL_Point> snake;   // cabeza = front
    SDL_Point fruit{5,5};
    std::mt19937 rng{12345};
    bool game_over = false;
    bool wrap_edges = false;       // si quieres wrap, cambia a true

    int wrap(int v, int m) { return (v % m + m) % m; }
    bool out_of_bounds(int x, int y, int cols, int rows) {
        return x < 0 || y < 0 || x >= cols || y >= rows;
    }

    void reset(GameContext& ctx) {
        ctx.score = 0;
        snake.clear();
        snake.push_front(SDL_Point{3,7});
        snake.push_back(SDL_Point{2,7});
        snake.push_back(SDL_Point{1,7});
        dirx = 1; diry = 0;
        std::uniform_int_distribution<int> cx(0, ctx.cols-1), cy(0, ctx.rows-1);
        fruit = SDL_Point{cx(rng), cy(rng)};
        acc = 0.f; game_over = false;
    }

public:
    void init(GameContext& ctx) override {
        ctx.cols = 20; ctx.rows = 15; ctx.cell_px = 24;
        ctx.input = InputMap::defaults_snake();
        reset(ctx);
    }

    void update(GameContext& ctx, float dt) override {
        if (game_over) {
            if (ctx.input.down(ctx.eng, "restart")) reset(ctx);
            return;
        }

        // entrada (evita reversa directa)
        if (ctx.input.down(ctx.eng, "left")  && dirx!=1)  { dirx=-1; diry=0; }
        if (ctx.input.down(ctx.eng, "right") && dirx!=-1) { dirx=1; diry=0; }
        if (ctx.input.down(ctx.eng, "up")    && diry!=1)  { dirx=0; diry=-1; }
        if (ctx.input.down(ctx.eng, "down")  && diry!=-1) { dirx=0; diry=1; }

        acc += dt;
        while (acc >= step) {
            acc -= step;
            SDL_Point head = snake.front();
            int nx = head.x + dirx, ny = head.y + diry;

            if (wrap_edges) {
                nx = wrap(nx, ctx.cols);
                ny = wrap(ny, ctx.rows);
            } else if (out_of_bounds(nx, ny, ctx.cols, ctx.rows)) {
                game_over = true; return;
            }

            // auto-colisión ⇒ GAME OVER
            for (auto& p : snake) if (p.x==nx && p.y==ny) { game_over = true; return; }

            bool grow = (nx == fruit.x && ny == fruit.y);
            snake.push_front(SDL_Point{nx, ny});
            if (!grow) snake.pop_back();
            else {
                std::uniform_int_distribution<int> cx(0, ctx.cols-1), cy(0, ctx.rows-1);
                fruit = SDL_Point{cx(rng), cy(rng)};
                ctx.score += 5;
            }
        }
    }

    void render(GameContext& ctx) override {
        auto& eng = ctx.eng;
        eng.clear(8,8,8,255);

        // grilla + borde del tablero
        draw_board_frame(ctx, SDL_Color{40,160,255,255}, SDL_Color{60,60,60,255});

        // fruta y serpiente dentro del área (0,0)-(cols*cell, rows*cell)
        int s = ctx.cell_px;
        eng.draw_brick(fruit.x*s, fruit.y*s, s, SDL_Color{255,80,80,255});
        for (size_t i=0;i<snake.size();++i) {
            auto p = snake[i];
            SDL_Color c = (i==0) ? SDL_Color{70,220,140,255} : SDL_Color{80,200,120,255};
            eng.draw_brick(p.x*s, p.y*s, s, c);
        }

        if (game_over) {
            eng.draw_text(8, 8, "SNAKE — GAME OVER. R reinicia. ESC sale.",
                          SDL_Color{255,120,120,255});
        } else {
            eng.draw_text(8, 8, "SNAKE — Flechas mueven. ESC sale. R reinicia.",
                          SDL_Color{255,255,255,255});
        }
        eng.draw_text(8, 32, "Score: " + std::to_string(ctx.score), SDL_Color{255,255,255,255});
        eng.present();
    }
};

std::unique_ptr<IGame> make_snake() { return std::make_unique<GameSnake>(); }
