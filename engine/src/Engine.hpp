#pragma once
#include <string>
#include <functional>
#include <SDL.h>
#include <SDL_ttf.h>

class Engine {
public:
    using UpdateFn = std::function<void(float)>; // dt en segundos
    using RenderFn = std::function<void(void)>;

    Engine(const char* title, int width, int height, int target_fps = 60);
    ~Engine();

    void setUpdate(UpdateFn fn);
    void setRender(RenderFn fn);
    void run();
    void quit();

    // Entrada
    bool isKeyDown(SDL_Scancode sc) const;

    // Render helpers
    void clear(Uint8 r, Uint8 g, Uint8 b, Uint8 a);
    void present();
    void draw_brick(int x, int y, int size, SDL_Color color);
    bool load_font(const std::string& path, int pt_size);
    void draw_text(int x, int y, const std::string& text, SDL_Color color);

    // Accesos
    SDL_Renderer* renderer() const { return renderer_; }
    int width()  const { return width_; }
    int height() const { return height_; }

private:
    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
    TTF_Font* font_ = nullptr;
    bool running_ = false;

    UpdateFn update_;
    RenderFn render_;

    int target_fps_ = 60;
    Uint32 frame_delay_ms_ = 16;
    int width_ = 0, height_ = 0;
};
