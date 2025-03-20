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
