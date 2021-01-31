cmake_minimum_required(VERSION 3.10)

project(noaftodo VERSION 1.4.5)

# use C++ 17
set(CXX_STANDART 17)
set(CXX_STANDART_REQUIRED True)

add_compile_options( -fpermissive -std=c++17 )

# Link ncurses
set(CURSES_NEED_WIDE True)
find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIR} "src/")

# Platform-specific stuff
message("Building for ${CMAKE_HOST_SYSTEM_NAME}")

if ("${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "Haiku")
	message("mqueue is off due to platform")
	set(_NO_MQUEUE 1)
	message("root check is off due to platform")
	set(_NO_ROOT_CHECK 1)
endif()

# Environment arguments
if ($ENV{NO_MQUEUE})
	message("mqueue is off by build flag")
	set(_NO_MQUEUE 1)
elseif (NOT("$ENV{NO_MQUEUE}" STREQUAL ""))
	message("mqueue is on by build flag")
	set(_NO_MQUEUE 0)
endif()

if ($ENV{NO_ROOT_CHECK})
	message("root check is off by build flag")
	set(_NO_ROOT_CHECK 1)
elseif (NOT("$ENV{NO_ROOT_CHECK}" STREQUAL ""))
	message("root check is on by build flag")
	set(_NO_ROOT_CHECK 0)
endif()

if ($ENV{DEBUG})
	message("debug flag on")
	set(_DEBUG 1)
endif()

if ("$ENV{PKGNAME}" STREQUAL "")
	set(_PKGNAME "${CMAKE_PROJECT_NAME}")
else()
	set(_PKGNAME "$ENV{PKGNAME}")
endif()

if (NOT ("$ENV{PKGROOT}" STREQUAL ""))
	set(_PKGROOT "$ENV{PKGROOT}")
endif()

# Find source files
file(GLOB_RECURSE _SOURCES src/*.cpp)

# Generate embedded code
add_custom_command(
	OUTPUT
	${CMAKE_CURRENT_BINARY_DIR}/gen/src/noaftodo_embed.cpp
	${CMAKE_CURRENT_BINARY_DIR}/gen/doc/noaftodo.man.gz
	COMMAND ${PROJECT_SOURCE_DIR}/tools/docgen.sh "${PROJECT_SOURCE_DIR}"
	COMMENT "Executing documentation and embedded code generation..."
	DEPENDS
	noaftodo.conf.template
	${_SOURCES})

# Declare an executable
add_executable(${CMAKE_PROJECT_NAME}
	${_SOURCES}
	${CMAKE_CURRENT_BINARY_DIR}/gen/src/noaftodo_embed.cpp)

target_link_libraries(${CMAKE_PROJECT_NAME} ${CURSES_LIBRARIES})

# Add nessesary build flags
# Explicitly set NCURSES_WIDECHAR
add_definitions( -DNCURSES_WIDECHAR ) 

# Set project version
add_definitions( -DLIST_V=${PROJECT_VERSION_MAJOR} )
add_definitions( -DCONF_V=${PROJECT_VERSION_MINOR} )
add_definitions( -DPATCH_V=${PROJECT_VERSION_PATCH} )

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

if (NOT ("$ENV{V_SUFFIX}" STREQUAL ""))
	add_definitions( -DV_SUFFIX="$ENV{V_SUFFIX}" )
endif()

# Declare installation targets
# Install binary
install(TARGETS ${CMAKE_PROJECT_NAME} DESTINATION "${_PKGROOT}/usr/bin/")
# Install LICENSE
install(FILES LICENSE DESTINATION "${_PKGROOT}/usr/share/licenses/${_PKGNAME}/")
# Install default config
install(FILES noaftodo.conf.template DESTINATION "${_PKGROOT}/etc/${_PKGNAME}/")
# Install scripts
install(DIRECTORY scripts DESTINATION "${_PKGROOT}/etc/${_PKGNAME}/")
# Install manual
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/gen/doc/noaftodo.man.gz DESTINATION "${_PKGROOT}/usr/share/man/man1/" RENAME "noaftodo.1.gz")
