#pragma once
#include <SDL.h>
#include <string>
#include <unordered_map>

class Engine; // forward decl

class InputMap {
public:
    void bind(const std::string& action, const std::string& key);
    bool down(const Engine& eng, const std::string& action) const;

    static SDL_Scancode parseKeyString(const std::string& s);
    static InputMap defaults_tetris();
    static InputMap defaults_snake();

private:
    std::unordered_map<std::string, SDL_Scancode> map_;
};

// --- Acciones canónicas para ambos juegos (evita typos) -------------
namespace Actions {
    // Comunes
    inline constexpr const char Left[]   = "left";
    inline constexpr const char Right[]  = "right";
    inline constexpr const char Up[]     = "up";
    inline constexpr const char Down[]   = "down";
    inline constexpr const char Pause[]  = "pause";
    inline constexpr const char Restart[] = "restart";

    // Snake
    inline constexpr const char Turbo[]         = "turbo";
    inline constexpr const char ToggleHead[]    = "toggle_head";
    inline constexpr const char TogglePalette[] = "toggle_palette";

    // Tetris
    inline constexpr const char SoftDrop[]  = "soft_drop";
    inline constexpr const char HardDrop[]  = "hard_drop";
    inline constexpr const char RotateCW[]  = "rotate_cw";
    inline constexpr const char RotateCCW[] = "rotate_ccw";
    inline constexpr const char Hold[]      = "hold";
}
