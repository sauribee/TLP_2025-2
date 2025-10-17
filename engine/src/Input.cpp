#include "Input.hpp"
#include "Engine.hpp"
#include <algorithm>
#include <cctype>   // std::tolower

static std::string lower(std::string s){
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c){ return char(std::tolower(c)); });
    return s;
}

void InputMap::bind(const std::string& action, const std::string& key) {
    map_[action] = parseKeyString(key);
}

bool InputMap::down(const Engine& eng, const std::string& action) const {
    auto it = map_.find(action);
    if (it == map_.end()) return false;
    return eng.isKeyDown(it->second);
}

SDL_Scancode InputMap::parseKeyString(const std::string& s_in) {
    std::string s = lower(s_in);
    if (s == "left")  return SDL_SCANCODE_LEFT;
    if (s == "right") return SDL_SCANCODE_RIGHT;
    if (s == "up")    return SDL_SCANCODE_UP;
    if (s == "down")  return SDL_SCANCODE_DOWN;
    if (s == "space") return SDL_SCANCODE_SPACE;
    if (s == "enter") return SDL_SCANCODE_RETURN;
    if (s.size()==1 && s[0]>='a' && s[0]<='z')
        return SDL_Scancode(SDL_SCANCODE_A + (s[0]-'a'));
    return SDL_SCANCODE_UNKNOWN;
}

InputMap InputMap::defaults_tetris() {
    InputMap m;
    m.bind("left",  "left");
    m.bind("right", "right");
    m.bind("down",  "down");   // soft drop
    m.bind("rotate","up");
    m.bind("pause", "p");
    m.bind("restart","r");
    return m;
}

InputMap InputMap::defaults_snake() {
    InputMap m;
    m.bind("left",  "left");
    m.bind("right", "right");
    m.bind("up",    "up");
    m.bind("down",  "down");
    m.bind("pause", "p");
    m.bind("restart","r");
    return m;
}
