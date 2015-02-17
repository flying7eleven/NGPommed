cmake_minimum_required( VERSION 2.8.7 )
project( "NGPommed" )

# do not allow in-source builds
if( CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR )
	message( FATAL_ERROR "In-source builds are not allowed." )
endif()

#
include( FindPkgConfig )

# set some specific compiler flags we need
set( CMAKE_CXX_FLAGS "-Wall -std=c++11" CACHE STRING "Flags used by the compiler during all build types." FORCE )

#
pkg_search_module( DBUS REQUIRED dbus-1 )
include_directories( SYSTEM ${DBUS_INCLUDE_DIRS} )

#
pkg_search_module( CONFUSE REQUIRED libconfuse )
include_directories( SYSTEM ${CONFUSE_INCLUDE_DIRS} )

#
set( POMMED_SOURCE_FILES ${POMMED_SOURCE_FILES} source/audio.c )
set( POMMED_SOURCE_FILES ${POMMED_SOURCE_FILES} source/beep.c )
set( POMMED_SOURCE_FILES ${POMMED_SOURCE_FILES} source/cd_eject.c )
set( POMMED_SOURCE_FILES ${POMMED_SOURCE_FILES} source/conffile.c )
set( POMMED_SOURCE_FILES ${POMMED_SOURCE_FILES} source/evdev.c )
set( POMMED_SOURCE_FILES ${POMMED_SOURCE_FILES} source/evloop.c )
set( POMMED_SOURCE_FILES ${POMMED_SOURCE_FILES} source/kbd_auto.c )
set( POMMED_SOURCE_FILES ${POMMED_SOURCE_FILES} source/pommed.c )
set( POMMED_SOURCE_FILES ${POMMED_SOURCE_FILES} source/sysfs_backlight.c )
set( POMMED_SOURCE_FILES ${POMMED_SOURCE_FILES} source/video.c )

#
set( POMMED_HEADER_FILES ${POMMED_HEADER_FILES} source/audio.h )
set( POMMED_HEADER_FILES ${POMMED_HEADER_FILES} source/beep.h )
set( POMMED_HEADER_FILES ${POMMED_HEADER_FILES} source/cd_eject.h )
set( POMMED_HEADER_FILES ${POMMED_HEADER_FILES} source/conffile.h )
set( POMMED_HEADER_FILES ${POMMED_HEADER_FILES} source/evdev.h )
set( POMMED_HEADER_FILES ${POMMED_HEADER_FILES} source/evloop.h )
set( POMMED_HEADER_FILES ${POMMED_HEADER_FILES} source/kbd_backlight.h )
set( POMMED_HEADER_FILES ${POMMED_HEADER_FILES} source/pommed.h )
set( POMMED_HEADER_FILES ${POMMED_HEADER_FILES} source/power.h )
set( POMMED_HEADER_FILES ${POMMED_HEADER_FILES} source/timerfd-syscalls.h )
set( POMMED_HEADER_FILES ${POMMED_HEADER_FILES} source/video.h )

#
add_executable( pommed ${POMMED_SOURCE_FILES} ${POMMED_HEADER_FILES} )
target_link_libraries( pommed ${DBUS_LIBRARIES} ${CONFUSE_LIBRARIES} )
install( TARGETS pommed RUNTIME DESTINATION bin )
