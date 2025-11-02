#include "Input.hpp"
#include "Engine.hpp"
#include <algorithm>
#include <cctype>   // std::tolower

// ---------------------------------------------------------------------
// Util: pasar a minúsculas (case-insensitive para nombres de teclas)
static std::string lower(std::string s){
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c){ return char(std::tolower(c)); });
    return s;
}

// ---------------------------------------------------------------------
// API
void InputMap::bind(const std::string& action, const std::string& key) {
    map_[action] = parseKeyString(key);
}

bool InputMap::down(const Engine& eng, const std::string& action) const {
    auto it = map_.find(action);
    if (it == map_.end()) return false;
    return eng.isKeyDown(it->second);
}

// ---------------------------------------------------------------------
// Conversión texto -> SDL_Scancode (extendido con teclas nuevas)
SDL_Scancode InputMap::parseKeyString(const std::string& s_in) {
    std::string s = lower(s_in);

    // Flechas
    if (s == "left")   return SDL_SCANCODE_LEFT;
    if (s == "right")  return SDL_SCANCODE_RIGHT;
    if (s == "up")     return SDL_SCANCODE_UP;
    if (s == "down")   return SDL_SCANCODE_DOWN;

    // Espacio / Enter / Escape
    if (s == "space" || s == "spacebar") return SDL_SCANCODE_SPACE;
    if (s == "enter" || s == "return")   return SDL_SCANCODE_RETURN;
    if (s == "escape" || s == "esc")     return SDL_SCANCODE_ESCAPE;

    // Shift (para Turbo en Snake, etc.)
    if (s == "lshift" || s == "shift")   return SDL_SCANCODE_LSHIFT;
    if (s == "rshift")                   return SDL_SCANCODE_RSHIFT;

    // Letras: "a".."z"
    if (s.size()==1 && s[0]>='a' && s[0]<='z')
        return SDL_Scancode(SDL_SCANCODE_A + (s[0]-'a'));

    return SDL_SCANCODE_UNKNOWN;
}

// ---------------------------------------------------------------------
// Mapas por defecto (Tetris + Snake) con acciones extendidas
InputMap InputMap::defaults_tetris() {
    InputMap m;

    // Básicas
    m.bind("left",    "left");
    m.bind("right",   "right");
    m.bind("down",    "down");   // soft drop continuo por flecha (si ya lo usas)
    m.bind("rotate",  "up");     // compatibilidad con tu lógica actual
    m.bind("pause",   "p");
    m.bind("restart", "r");
    m.bind("help", "h");  // <- añade esta línea


    // Extensiones (opcional usarlas en tu lógica):
    m.bind("soft_drop",  "s");       // adicional a "down"
    m.bind("hard_drop",  "space");
    m.bind("rotate_cw",  "x");
    m.bind("rotate_ccw", "z");
    m.bind("hold",       "a");

    return m;
}

InputMap InputMap::defaults_snake() {
    InputMap m;

    // Movimiento + control
    m.bind("left",    "left");
    m.bind("right",   "right");
    m.bind("up",      "up");
    m.bind("down",    "down");
    m.bind("pause",   "p");
    m.bind("restart", "r");

    // Extensiones de front-end
    m.bind("turbo",          "lshift"); // acelerar mientras se mantiene
    m.bind("toggle_head",    "h");      // alterna cabeza (arrow/square)
    m.bind("toggle_palette", "c");      // cambia paleta (classic/neon/rainbow)

    return m;
}
