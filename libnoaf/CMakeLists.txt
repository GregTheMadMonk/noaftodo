cmake_minimum_required(VERSION 3.10)

set( _NAME noaf )
project("lib${_NAME}" VERSION 1.0.0)

# use C++ 17
set(CXX_STANDART 17)
set(CXX_STANDART_REQUIRED True)

add_compile_options( -fpermissive -std=c++17)

# Link ncurses
set(CURSES_NEED_WIDE True)
find_package(Curses REQUIRED)

# include directories
include_directories(src/)

# envvar parameters parser
if (NOT ("$ENV{PKGROOT}" STREQUAL ""))
	set(_PKGROOT "$ENV{PKGROOT}")
endif()

file(GLOB_RECURSE _SOURCES src/*.cpp)

add_library(${_NAME} SHARED
	${_SOURCES})

# Add nessesary build flags
# Explicitly set NCURSES_WIDECHAR
add_definitions( -DNCURSES_WIDECHAR ) 
