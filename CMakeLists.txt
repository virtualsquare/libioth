cmake_minimum_required(VERSION 3.13)
project("libioth"
		DESCRIPTION "Internet of Threads library"
		HOMEPAGE_URL "https://github.com/virtualsquare/libioth"
    VERSION 0.1.0
    LANGUAGES C)

include(GNUInstallDirs)
include(CheckIncludeFile)
include(CheckSymbolExists)

# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_FORTIFY_SOURCE=2 -O2 -pedantic -Wall -Wextra")
set(SYSTEM_IOTH_PATH ${CMAKE_INSTALL_FULL_LIBDIR}/ioth)

set(LIBS_REQUIRED fduserdata vdeplug)
set(HEADERS_REQUIRED nlinline+.h fduserdata.h libvdeplug.h)
set(CMAKE_REQUIRED_QUIET TRUE)

foreach(THISLIB IN LISTS LIBS_REQUIRED)
  find_library(LIB${THISLIB}_OK ${THISLIB})
  if(NOT LIB${THISLIB}_OK)
    message(FATAL_ERROR "library lib${THISLIB} not found")
  endif()
endforeach(THISLIB)

foreach(HEADER IN LISTS HEADERS_REQUIRED)
  check_include_file(${HEADER} ${HEADER}_OK)
  if(NOT ${HEADER}_OK)
    message(FATAL_ERROR "header file ${HEADER} not found")
  endif()
endforeach(HEADER)

add_definitions(-D_GNU_SOURCE)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_library(ioth SHARED ioth.c)
target_link_libraries(ioth -ldl -lfduserdata -lpthread)
set_target_properties(ioth PROPERTIES VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR})
install(TARGETS ioth DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES ioth.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# configure_file(picoxnet.pc.in picoxnet.pc @ONLY)
configure_file(config.h.in config.h)

add_subdirectory(test)
add_subdirectory(modules)
# add_subdirectory(man)

add_custom_target(uninstall
  "${CMAKE_COMMAND}" -P "${PROJECT_SOURCE_DIR}/Uninstall.cmake")

