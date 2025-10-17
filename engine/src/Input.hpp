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
