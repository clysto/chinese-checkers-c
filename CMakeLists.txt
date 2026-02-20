cmake_minimum_required(VERSION 3.15)

project(Checkers LANGUAGES C)

set(CMAKE_C_FLAGS_RELEASE "-O3")

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

if (ZIG_CROSS_COMPILE_LINUX)
    message(STATUS "Cross-compiling for Linux")
    target_include_directories(checkers_gui PRIVATE ${CMAKE_SOURCE_DIR}/external/raylib-5.5_linux_amd64/include)
    target_link_libraries(checkers_gui PRIVATE ${CMAKE_SOURCE_DIR}/external/raylib-5.5_linux_amd64/lib/libraylib.a)
elseif(ZIG_CROSS_COMPILE_WINDOWS)
    message(STATUS "Cross-compiling for Windows")
    target_include_directories(checkers_gui PRIVATE ${CMAKE_SOURCE_DIR}/external/raylib-5.5_win64_mingw-w64/include)
    target_link_libraries(checkers_gui PRIVATE ${CMAKE_SOURCE_DIR}/external/raylib-5.5_win64_mingw-w64/lib/raylib.dll)
else()
    target_include_directories(checkers_gui PRIVATE ${CMAKE_SOURCE_DIR}/external/raylib-5.5_macos/include)
    target_link_libraries(checkers_gui PRIVATE ${CMAKE_SOURCE_DIR}/external/raylib-5.5_macos/lib/libraylib.dylib)
endif()
