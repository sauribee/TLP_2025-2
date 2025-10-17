#include "Engine.hpp"
#include "IGame.hpp"
#include "GameFactory.hpp"
#include <string>
#include <iostream>

int main(int argc, char** argv) {
    std::string mode = "tetris"; // por defecto
    if (argc >= 3 && std::string(argv[1]) == "--demo") mode = argv[2];

    try {
        Engine eng(("Brik Engine — " + mode).c_str(), 640, 480, 60);
        eng.load_font("assets/DejaVuSans.ttf", 16);

        GameContext ctx{eng};
        auto game = make_game(mode);
        game->init(ctx);

        eng.setUpdate([&](float dt){ game->update(ctx, dt); });
        eng.setRender([&](){ game->render(ctx); });

        eng.run();
    } catch (const std::exception& ex) {
        std::cerr << "Fatal: " << ex.what() << "\n";
        return 1;
    }
    return 0;
}
