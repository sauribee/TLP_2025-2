#include "IGame.hpp"
#include "Engine.hpp"
#include "Input.hpp"
#include <algorithm>
#include <memory>
#include <string>
#include <SDL.h> // SDL_RenderGetLogicalSize, SDL_GetRendererOutputSize

class GameBrick : public IGame {
    int   bx = 0, by = 0;
    int   size = 40;           // píxeles del ladrillo
    float speed = 220.0f;      // px/seg
    int   winW = 640, winH = 480;

    void refresh_bounds(GameContext& ctx) {
        int lw = 0, lh = 0;
        SDL_Renderer* r = ctx.eng.renderer();
        if (r) {
            SDL_RenderGetLogicalSize(r, &lw, &lh);
            if (lw > 0 && lh > 0) { winW = lw; winH = lh; return; }
            int ow = 0, oh = 0;
            if (SDL_GetRendererOutputSize(r, &ow, &oh) == 0 && ow > 0 && oh > 0) {
                winW = ow; winH = oh; return;
            }
        }
        // fallback razonable
        winW = 640; winH = 480;
    }

public:
    void init(GameContext& ctx) override {
    refresh_bounds(ctx);
    bx = winW/2 - size/2;
    by = winH/2 - size/2;

    // Opción A: mapa que ya trae left/right/up/down
    ctx.input = InputMap::defaults_snake();

    // Opción B (extra seguro): aseguramos los binds explícitos de flechas
    ctx.input.bind("left",  "LEFT");
    ctx.input.bind("right", "RIGHT");
    ctx.input.bind("up",    "UP");     // <- importante
    ctx.input.bind("down",  "DOWN");

    ctx.score = 0;
    }

    void update(GameContext& ctx, float dt) override {
        refresh_bounds(ctx); // por si la ventana cambia
        if (ctx.input.down(ctx.eng, "left"))  bx = std::max(0, bx - int(speed * dt));
        if (ctx.input.down(ctx.eng, "right")) bx = std::min(winW - size, bx + int(speed * dt));
        if (ctx.input.down(ctx.eng, "up"))    by = std::max(0, by - int(speed * dt));
        if (ctx.input.down(ctx.eng, "down"))  by = std::min(winH - size, by + int(speed * dt));
    }

    void render(GameContext& ctx) override {
        ctx.eng.clear(14,14,14,255);
        ctx.eng.draw_brick(bx, by, size, SDL_Color{255,200,60,255});
        ctx.eng.draw_text(8, 8,  "DEMO BRICK — Flechas mueven. ESC sale.", SDL_Color{255,255,255,255});
        ctx.eng.draw_text(8, 32, "pos=(" + std::to_string(bx) + "," + std::to_string(by) + ")", SDL_Color{200,200,200,255});
        ctx.eng.present();
    }
};

std::unique_ptr<IGame> make_brick() { return std::make_unique<GameBrick>(); }
