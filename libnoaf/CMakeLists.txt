cmake_minimum_required(VERSION 3.10)

set( _NAME noaf )
project( "lib${_NAME}" VERSION 1.0.0 )

# Use C++ 17
set( CXX_STANDART 17 )
set( CXX_STANDART_REQUIRED True )

add_compile_options( -fpermissive -std=c++17 )

# Require ncurses
set( CURSES_NEED_WIDE True )
find_package( Curses REQUIRED )

# Require Qt5
find_package( Qt5 COMPONENTS Core Gui Widgets REQUIRED )

# Include index
include_directories( src/core )

# Envvar controlled arguments
# TODO

# Find source files
file( GLOB_RECURSE _SOURCES src/core/*.cpp )
file( GLOB_RECURSE _SOURCES_CUI src/cui/*.cpp )
file( GLOB_RECURSE _SOURCES_GUI src/gui/*.cpp )

# Declare the library
# NOAF core
add_library( ${_NAME} SHARED
	${_SOURCES} )

# NOAF CUI
add_library( ${_NAME}cui SHARED
	${_SOURCES_CUI} )
target_link_libraries( ${_NAME}cui ${CURSES_LIBRARIES} )

# NOAF GUI
SET( CMAKE_AUTOMOC TRUE )
add_library( ${_NAME}gui SHARED
	${_SOURCES_GUI} )
target_link_libraries( ${_NAME}gui Qt5::Core Qt5::Gui Qt5::Widgets )
SET( CMAKE_AUTOMOC FALSE )

# Add nessesary build flags
# Explicitly set NCURSES_WIDECHAR
add_definitions( -DNCURSES_WIDECHAR ) 

# Pass version to code
# TODO

# Build demos
file( GLOB_RECURSE _DEMOS demos/*.cpp )
include_directories( src/cui src/gui )
foreach( _DEMO ${_DEMOS} )
	get_filename_component( _DEMO_NAME ${_DEMO} NAME_WE )
	message( STATUS "${_DEMO_NAME}" )
	add_executable( ${_DEMO_NAME} ${_DEMO} )
	if ( "${_DEMO_NAME}" STREQUAL "noafui" )
		message( "Building noafui" )
		target_link_libraries( ${_DEMO_NAME} noaf dl )
	else()
		target_link_libraries( ${_DEMO_NAME} noaf noafcui noafgui )
	endif()
endforeach()

# Install the library
# TODO
