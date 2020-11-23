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

set(LIBS_REQUIRED fduserdata)
set(HEADERS_REQUIRED nlinline+.h fduserdata.h)
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

add_library(ioth SHARED libioth.c)
target_link_libraries(ioth -ldl -lfduserdata -lpthread)
set_target_properties(ioth PROPERTIES VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR})
install(TARGETS ioth DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES libioth.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

add_library(ioth_kernel SHARED ioth_kernel.c)
set_target_properties(ioth_kernel PROPERTIES PREFIX "")
install(TARGETS ioth_kernel DESTINATION ${SYSTEM_IOTH_PATH})

find_library(LIBVDESTACK_OK vdestack)
check_include_file("vdestack.h" INCLUDEVDESTACK_OK)
if (LIBVDESTACK_OK AND INCLUDEVDESTACK_OK)
	add_library(ioth_vdestack SHARED ioth_vdestack.c)
	set_target_properties(ioth_vdestack PROPERTIES PREFIX "")
	target_link_libraries(ioth_vdestack vdestack)
	install(TARGETS ioth_vdestack DESTINATION ${SYSTEM_IOTH_PATH})
endif()

find_library(LIBPICOX_OK picoxnet)
check_include_file("picoxnet.h" INCLUDEPICOX_OK)
if (LIBPICOX_OK AND INCLUDEPICOX_OK)
	add_library(ioth_picox SHARED ioth_picox.c)
	set_target_properties(ioth_picox PROPERTIES PREFIX "")
	target_link_libraries(ioth_picox picoxnet)
	install(TARGETS ioth_picox DESTINATION ${SYSTEM_IOTH_PATH})
endif()

add_executable(iothtest_server iothtest_server.c)
target_link_libraries(iothtest_server ioth pthread)

add_executable(iothtest_client iothtest_client.c)
target_link_libraries(iothtest_client ioth pthread)

# configure_file(picoxnet.pc.in picoxnet.pc @ONLY)
configure_file(config.h.in config.h)

# add_subdirectory(man)

add_custom_target(uninstall
  "${CMAKE_COMMAND}" -P "${PROJECT_SOURCE_DIR}/Uninstall.cmake")

