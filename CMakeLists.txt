cmake_minimum_required(VERSION 3.10)

include( CheckIncludeFile )

project(noaftodo VERSION 2.0.0)

# Use C++ 17
set(CXX_STANDART 17)
set(CXX_STANDART_REQUIRED True)

add_compile_options( -fpermissive -std=c++17 )

# Compile and link libnoaf
add_subdirectory( libnoaf )

# Link ncurses
set( NCURSES_NEED_WIDE True )
find_package( Curses REQUIRED )

# Include index
include_directories( ${CURSES_INCLUDE_DIR} "src/" "libnoaf/src" ) # TODO: export libnoaf includes and use them

# Check for needed headers
CHECK_INCLUDE_FILE( "mqueue.h" _HAS_MQUEUE )
if ( NOT _HAS_MQUEUE )
	message("Fail! Using a replacement...")
	set( _NO_MQUEUE 1 )
endif()

# Platform-specific stuff
if ( "${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "Haiku" )
	message( "root check is useless on Haiku since it's a single-user system. Disabling...." )
	set( _NO_ROOT_CHECK 1 )
endif()

# Envvar controlled arguments
if ( $ENV{NO_MQUEUE} )
	message( "mqueue is off by build flag. Using a replacement..." )
	set( _NO_MQUEUE 1 )
elseif ( NOT ( "$ENV{NO_MQUEUE}" STREQUAL "" ) )
	message( "mqueue is forced on by build flag." )
	set( _NO_MQUEUE 0 )
endif()

if ( $ENV{NO_ROOT_CHECK} )
	message( "root check is off by build flag." )
	set( _NO_ROOT_CHECK 1 )
elseif ( NOT ( "$ENV{NO_ROOT_CHECK}" STREQUAL "" ) )
	message( "root check is on by build flag." )
	set( _NO_ROOT_CHECK 0 )
endif()

if ( "$ENV{PKGNAME}" STREQUAL "" )
	set( _PKGNAME "${CMAKE_PROJECT_NAME}" )
else()
	set( _PKGNAME "$ENV{PKGNAME}" )
endif()

if ( NOT ( "$ENV{PKGROOT}" STREQUAL "" ) )
	set( _PKGROOT "$ENV{PKGROOT}" )
endif()

# Find source files
file( GLOB_RECURSE _SOURCES src/*.cpp )

# Declare an executable
add_executable( ${CMAKE_PROJECT_NAME} ${_SOURCES} )
target_link_libraries( ${CMAKE_PROJECT_NAME} ${CURSES_LIBRARIES} noaf )

# Explicitly set NCURSES_WIDECHAR
add_definitions( -DNCURSES_WIDECHAR )

if ( ${_NO_MQUEUE} )
	add_definitions( -DNO_MQUEUE )
else()
	target_link_libraries( ${CMAKE_PROJECT_NAME} rt )
endif()

if ( ${_NO_ROOT_CHECK} )
	add_definitions( -DNO_ROOT_CHECK )
endif()

# Pass version to code
# TODO

# Add version suffix
# TODO

# Generate docs
# TODO

# Install the program
# TODO
