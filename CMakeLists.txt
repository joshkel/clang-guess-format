cmake_minimum_required(VERSION 3.4)
project(ClangGuessFormat)

set(Clang_DIR /usr/lib/cmake/clang-12/ CACHE FILEPATH "Clang location")
find_package(Clang REQUIRED)

set(CMAKE_CXX_STANDARD 17)
add_compile_options(-Wall -Wextra -Wno-unused-parameter)

add_executable(${PROJECT_NAME} ClangGuessFormat.cpp)
include_directories(SYSTEM ${CLANG_INCLUDE_DIRS})
link_directories(${CLANG_LIBRARY_DIRS})
target_link_libraries(${PROJECT_NAME} clangTooling)
