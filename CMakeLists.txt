cmake_minimum_required(VERSION 3.10)

project(noaftodo)

# use C++ 17
set(CXX_STANDART 17)
set(CXX_STANDART_REQUIRED True)

add_compile_options( -DNCURSES_WIDECHAR -fpermissive -std=c++17 )

# Link ncurses
set(CURSES_NEED_WIDE True)
find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIR})

# Environment arguments
if ($ENV{NO_MQUEUE})
	message("mqueue is off by build flag")
	set(_NO_MQUEUE 1)
endif()

if ($ENV{NO_ROOT_CHECK})
	message("root check is off by build flag")
	set(_NO_ROOT_CHECK 1)
endif()

if ($ENV{DEBUG})
	message("debug flag on")
	set(_DEBUG 1)
endif()

if ($ENV{PKGNAME})
	set(_PKGNAME $ENV{PKGNAME})
else()
	set(_PKGNAME noaftodo)
endif()

# Platform-specific stuff
message("Building for ${CMAKE_HOST_SYSTEM_NAME}")

if (${CMAKE_HOST_SYSTEM_NAME} EQUAL "Haiku")
	message("mqueue is off due to platform")
	set(_NO_MQUEUE 1)
	message("root check is off due to platform")
	set(_NO_ROOT_CHECK 1)
endif()

# Generate embedded code
message("Executing documentation and embedded code generation...")
execute_process(COMMAND ${PROJECT_SOURCE_DIR}/tools/docgen.sh "${PROJECT_SOURCE_DIR}")

# Find source files
file(GLOB_RECURSE _HEADERS src/*.h)
file(GLOB_RECURSE _SOURCES src/*.cpp)
file(GLOB_RECURSE _HEADERS_GEN ${CMAKE_CURRENT_BINARY_DIR}/gen/src/*.h)
file(GLOB_RECURSE _SOURCES_GEN ${CMAKE_CURRENT_BINARY_DIR}/gen/src/*.cpp)

add_executable(${CMAKE_PROJECT_NAME} ${_SOURCES} ${_SOURCES_GEN})
target_link_libraries(${CMAKE_PROJECT_NAME} ${CURSES_LIBRARIES})

# Add nessesary build flags
if (${_NO_MQUEUE})
	add_definitions( -DNO_MQUEUE )
else()
	target_link_libraries(${CMAKE_PROJECT_NAME} rt)
endif()

if (${_NO_ROOT_CHECK})
	add_definitions( -DNO_ROOT_CHECK )
endif()

if (${_DEBUG})
	add_compile_options( -g3 )
endif()

if ($ENV{V_SUFFIX})
	add_definitions( -DV_SUFFIX="${ENV{V_SUFFIX}" )
endif()
