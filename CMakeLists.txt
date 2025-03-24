cmake_minimum_required(VERSION 3.15)

project(Checkers LANGUAGES C)

add_executable(checkers
    src/main.c
    src/checkers.c
    src/checkers.h
    src/list.h
    src/search.c
    src/search.h
)

add_executable(checkers_gui
    src/checkers.c
    src/checkers.h
    src/list.h
    src/search.c
    src/search.h
    src/gui/main.c
)

find_package(raylib 5.0 REQUIRED)
target_link_libraries(checkers_gui PRIVATE raylib)
