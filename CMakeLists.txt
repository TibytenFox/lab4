cmake_minimum_required(VERSION 3.14)
project(LazySequence VERSION 1.0.0 LANGUAGES CXX)

include(FetchContent)
FetchContent_Declare(
    data_structures
    GIT_REPOSITORY  https://github.com/TibytenFox/sequence
    GIT_TAG         main
)
FetchContent_MakeAvailable(data_structures)

add_executable(app main.cpp)
target_link_libraries(app PRIVATE sequence)
target_compile_features(app PRIVATE cxx_std_17)

add_executable(test test.cpp)
target_link_libraries(test PRIVATE sequence)
target_compile_features(test PRIVATE cxx_std_17)
