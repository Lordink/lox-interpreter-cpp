cmake_minimum_required(VERSION 3.13)
project(codecrafters-interpreter)

set(CMAKE_CXX_STANDARD 23) # Enable the C++23 standard

# Separate lib code from main
file(GLOB_RECURSE LIB_SOURCE_FILES src/*.cpp src/*.hpp)
list(REMOVE_ITEM LIB_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)

add_executable(interpreter src/main.cpp ${LIB_SOURCE_FILES})

find_package(Catch2 3 CONFIG)

file(GLOB_RECURSE TEST_FILES tests/*.cpp)
if(TEST_FILES AND Catch2_FOUND)
    enable_testing()
    add_executable(interp_tests ${TEST_FILES} ${LIB_SOURCE_FILES})
    target_link_libraries(interp_tests PRIVATE Catch2::Catch2WithMain)

    include(CTest)
    list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
    include(Catch)
    catch_discover_tests(interp_tests)
endif()