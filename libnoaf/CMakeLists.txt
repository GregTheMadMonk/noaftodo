cmake_minimum_required(VERSION 3.10)

set( _NAME noaf )
project("lib${_NAME}" VERSION 1.0.0)

# Use C++ 17
set(CXX_STANDART 17)
set(CXX_STANDART_REQUIRED True)

add_compile_options( -fpermissive -std=c++17 )

# Link ncurses
set(CURSES_NEED_WIDE True)
find_package(Curses REQUIRED)

# Include index
include_directories(src/)

# Envvar controlled arguments
# TODO

# Find source files
file(GLOB_RECURSE _SOURCES src/*.cpp)

# Declare the library
add_library(${_NAME} SHARED
	${_SOURCES})

# Add nessesary build flags
# Explicitly set NCURSES_WIDECHAR
add_definitions( -DNCURSES_WIDECHAR ) 

# Pass version to code
# TODO

# Install the library
# TODO
