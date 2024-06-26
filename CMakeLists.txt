cmake_minimum_required(VERSION 3.27)
project(full_board_solver)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(external/raylib-5.0)
add_subdirectory(external/raylib-cpp-5.0.1)

if (WIN32)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -stdlib=libc++ -lc++abi")
    endif ()
endif ()

if (EMSCRIPTEN)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -sUSE_GLFW=3 -sASSERTIONS=1 -sWASM=1 -sGL_ENABLE_GET_PROC_ADDRESS=1 -sALLOW_MEMORY_GROWTH --shell-file=../src/index.html")
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
endif ()

add_executable(full_board_solver
        src/main.cpp
        src/raygui.c)
target_include_directories(full_board_solver SYSTEM PRIVATE
        external/thread-pool-4.0.1/include
        external/raygui-4.0/include)
target_link_libraries(full_board_solver raylib raylib_cpp)
