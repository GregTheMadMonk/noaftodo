cmake_minimum_required(VERSION 3.10)

set( _NAME noaf )
project( "lib${_NAME}" VERSION 1.0.0 )

# Use C++ 17
set( CXX_STANDART 17 )
set( CXX_STANDART_REQUIRED True )

add_compile_options( -fpermissive -std=c++17 )

# Link ncurses
set( CURSES_NEED_WIDE True )
find_package( Curses REQUIRED )

# Include index
include_directories( src/ )

# Envvar controlled arguments
# TODO

# Find source files
file( GLOB_RECURSE _SOURCES src/*.cpp )

# Declare the library
add_library( ${_NAME} SHARED
	${_SOURCES} )

# Add nessesary build flags
# Explicitly set NCURSES_WIDECHAR
add_definitions( -DNCURSES_WIDECHAR ) 

# Pass version to code
# TODO

# Build demos
file( GLOB_RECURSE _DEMOS demos/*.cpp )
foreach( _DEMO ${_DEMOS} )
	get_filename_component( _DEMO_NAME ${_DEMO} NAME_WE )
	message( STATUS "${_DEMO_NAME}" )
	add_executable( ${_DEMO_NAME} ${_DEMO} )
	target_link_libraries( ${_DEMO_NAME} ${CURSES_LIBRARIES} noaf )
endforeach()

# Install the library
# TODO
