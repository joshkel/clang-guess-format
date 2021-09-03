cmake_minimum_required(VERSION 3.4)
project(ClangGuessFormat)

find_package(Clang REQUIRED)
message(STATUS "Using ${Clang_DIR}/ClangConfig.cmake")

add_executable(${PROJECT_NAME} ClangGuessFormat.cpp)
include_directories(${CLANG_INCLUDE_DIRS})
link_directories(${CLANG_LIBRARY_DIRS})
target_link_libraries(${PROJECT_NAME} clangTooling)
