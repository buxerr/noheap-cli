cmake_minimum_required(VERSION 3.16)
project(noheap_cli LANGUAGES CXX)

add_library(tinycli INTERFACE)
add_library(noheap_cli::tinycli ALIAS tinycli)
target_include_directories(tinycli INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_compile_features(tinycli INTERFACE cxx_std_11)

add_executable(tinycli-tests tests/test_cli.cpp)
target_link_libraries(tinycli-tests PRIVATE tinycli)

add_executable(tinycli-example examples/basic_cli.cpp)
target_link_libraries(tinycli-example PRIVATE tinycli)
set_target_properties(tinycli-tests tinycli-example PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS OFF)

if(MSVC)
    target_compile_options(tinycli-tests PRIVATE /W4 /WX)
    target_compile_options(tinycli-example PRIVATE /W4 /WX)
else()
    target_compile_options(tinycli-tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
    target_compile_options(tinycli-example PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

enable_testing()
add_test(NAME tinycli-tests COMMAND tinycli-tests)
