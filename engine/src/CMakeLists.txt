cmake_minimum_required(VERSION 3.16)
project(brik_engine CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB SRC
    "src/*.cpp"
)
add_executable(brik_engine ${SRC})
target_include_directories(brik_engine PRIVATE src)

# SDL2 y SDL2_ttf — intentamos varias rutas para portabilidad
find_package(SDL2 QUIET CONFIG)
if (SDL2_FOUND)
  target_link_libraries(brik_engine PRIVATE SDL2::SDL2)
else()
  find_package(PkgConfig QUIET)
  if (PkgConfig_FOUND)
    pkg_check_modules(SDL2 REQUIRED sdl2)
    target_include_directories(brik_engine PRIVATE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(brik_engine PRIVATE ${SDL2_LIBRARIES})
  else()
    target_link_libraries(brik_engine PRIVATE SDL2) # fallback por nombre
  endif()
endif()

find_package(SDL2_ttf QUIET CONFIG)
if (SDL2_ttf_FOUND)
  target_link_libraries(brik_engine PRIVATE SDL2_ttf::SDL2_ttf)
else()
  find_package(PkgConfig QUIET)
  if (PkgConfig_FOUND)
    pkg_check_modules(SDL2TTF REQUIRED SDL2_ttf)
    target_include_directories(brik_engine PRIVATE ${SDL2TTF_INCLUDE_DIRS})
    target_link_libraries(brik_engine PRIVATE ${SDL2TTF_LIBRARIES})
  else()
    target_link_libraries(brik_engine PRIVATE SDL2_ttf) # fallback por nombre
  endif()
endif()
