#include "Engine.hpp"
#include <stdexcept>

Engine::Engine(const char* title, int width, int height, int target_fps)
    : target_fps_(target_fps), width_(width), height_(height) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0)
        throw std::runtime_error(std::string("SDL_Init failed: ") + SDL_GetError());
    if (TTF_Init() != 0) {
        SDL_Quit();
        throw std::runtime_error(std::string("TTF_Init failed: ") + TTF_GetError());
    }

    window_ = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                               width, height, SDL_WINDOW_SHOWN);
    if (!window_) {
        TTF_Quit(); SDL_Quit();
        throw std::runtime_error(std::string("SDL_CreateWindow failed: ") + SDL_GetError());
    }

    renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer_) {
        SDL_DestroyWindow(window_); TTF_Quit(); SDL_Quit();
        throw std::runtime_error(std::string("SDL_CreateRenderer failed: ") + SDL_GetError());
    }

    frame_delay_ms_ = static_cast<Uint32>(1000.0f / (float)target_fps_);
}

Engine::~Engine() {
    if (font_) TTF_CloseFont(font_);
    if (renderer_) SDL_DestroyRenderer(renderer_);
    if (window_) SDL_DestroyWindow(window_);
    TTF_Quit();
    SDL_Quit();
}

void Engine::setUpdate(UpdateFn fn) { update_ = std::move(fn); }
void Engine::setRender(RenderFn fn) { render_ = std::move(fn); }

bool Engine::isKeyDown(SDL_Scancode sc) const {
    const Uint8* ks = SDL_GetKeyboardState(nullptr);
    return ks && ks[sc];
}

void Engine::clear(Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
    SDL_SetRenderDrawColor(renderer_, r, g, b, a);
    SDL_RenderClear(renderer_);
}

void Engine::present() { SDL_RenderPresent(renderer_); }

void Engine::draw_brick(int x, int y, int size, SDL_Color color) {
    SDL_Rect rect{ x, y, size, size };
    SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer_, &rect);
    SDL_SetRenderDrawColor(renderer_, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer_, &rect);
}

bool Engine::load_font(const std::string& path, int pt_size) {
    if (font_) { TTF_CloseFont(font_); font_ = nullptr; }
    font_ = TTF_OpenFont(path.c_str(), pt_size);
    return font_ != nullptr;
}

void Engine::draw_text(int x, int y, const std::string& text, SDL_Color color) {
    if (!font_) return;
    SDL_Surface* surf = TTF_RenderUTF8_Blended(font_, text.c_str(), color);
    if (!surf) return;
    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer_, surf);
    SDL_FreeSurface(surf);
    if (!tex) return;
    int w, h; SDL_QueryTexture(tex, nullptr, nullptr, &w, &h);
    SDL_Rect dst{ x, y, w, h };
    SDL_RenderCopy(renderer_, tex, nullptr, &dst);
    SDL_DestroyTexture(tex);
}

void Engine::run() {
    running_ = true;
    Uint64 freq = SDL_GetPerformanceFrequency();
    Uint64 last = SDL_GetPerformanceCounter();

    while (running_) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running_ = false;
            if (e.type == SDL_KEYDOWN && e.key.keysym.scancode == SDL_SCANCODE_ESCAPE) running_ = false;
        }

        Uint64 now = SDL_GetPerformanceCounter();
        float dt = float(now - last) / float(freq);
        if (dt > 0.25f) dt = 0.25f;
        last = now;

        if (update_) update_(dt);
        if (render_) render_();
        // Con VSYNC no hace falta SDL_Delay.
    }
}

void Engine::quit() { running_ = false; }
