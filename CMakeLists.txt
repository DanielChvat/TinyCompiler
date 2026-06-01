cmake_minimum_required(VERSION 3.20)

project(tinycc LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "LLVM include dirs: ${LLVM_INCLUDE_DIRS}")
message(STATUS "LLVM definitions: ${LLVM_DEFINITIONS}")

include_directories(include)

add_executable(
    tinycc
    src/main.cpp
    src/Lexer.cpp
    src/Parser.cpp
    src/CodeGen.cpp
)

target_include_directories(tinycc PRIVATE
    include
    ${LLVM_INCLUDE_DIRS}
)

target_compile_definitions(tinycc PRIVATE
    ${LLVM_DEFINITIONS}
)

target_link_libraries(tinycc PRIVATE LLVM)