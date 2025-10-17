#pragma once
#include "Input.hpp"

// forward decl (también llega via Input.hpp, pero es seguro repetir)
class Engine;

struct GameContext {
    Engine& eng;
    int cols = 10;
    int rows = 20;
    int cell_px = 24;
    InputMap input;
    int score = 0;
};

class IGame {
public:
    virtual ~IGame() {}
    virtual void init(GameContext&) = 0;
    virtual void update(GameContext&, float dt) = 0; // usa ticks fijos internamente
    virtual void render(GameContext&) = 0;
};
