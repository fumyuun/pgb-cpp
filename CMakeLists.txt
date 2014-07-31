cmake_minimum_required(VERSION 2.8.12)

project(pgb)

include("wubwubcmake/wub_find_modules.cmake")

find_package(SDL2 REQUIRED)
find_package(Boost COMPONENTS thread system chrono REQUIRED)

file(GLOB sources "src/*.cpp")
add_executable(pgb ${sources})
target_link_libraries(pgb PRIVATE ${SDL2_LIBRARIES} ${Boost_LIBRARIES})
