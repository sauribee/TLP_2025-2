#include "GameFactory.hpp"
#include <algorithm>
#include <cctype>   // tolower
#include <memory>

// fwd (definidas en sus .cpp)
std::unique_ptr<IGame> make_tetris();
std::unique_ptr<IGame> make_snake();
std::unique_ptr<IGame> make_brick();   // <-- AÑADIR ESTA LÍNEA

std::unique_ptr<IGame> make_game(const std::string& rules_name) {
    std::string s = rules_name;
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    if (s.find("tetris") != std::string::npos) return make_tetris();
    if (s.find("snake")  != std::string::npos) return make_snake();
    if (s.find("brick")  != std::string::npos) return make_brick();
    return make_tetris();
}
