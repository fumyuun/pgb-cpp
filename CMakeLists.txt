cmake_minimum_required(VERSION 2.8.12)

project(pgb)

find_package(SDL REQUIRED)

file(GLOB sources "src/*.cpp")
add_executable(pgb ${sources})
target_link_libraries(pgb PRIVATE ${SDL_LIBRARY})
