#pragma once
#include <memory>
#include <string>
#include "IGame.hpp"

std::unique_ptr<IGame> make_game(const std::string& rules_name);
