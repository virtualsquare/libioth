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

add_library(ioth_kernel-r SHARED ioth_kernel.c)
set_target_properties(ioth_kernel-r PROPERTIES PREFIX "")
install(TARGETS ioth_kernel-r DESTINATION ${SYSTEM_IOTH_PATH})
ADD_CUSTOM_TARGET(ioth_kernel_n.so ALL DEPENDS ioth_kernel-r
	COMMAND ${CMAKE_COMMAND} -E create_symlink ioth_kernel-r.so ioth_kernel_n.so)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ioth_kernel_n.so DESTINATION ${SYSTEM_IOTH_PATH})

find_library(LIBVDEPLUG_OK vdeplug)
check_include_file("libvdeplug.h" INCLUDEVDEPLUG_OK)
if (LIBVDEPLUG_OK AND INCLUDEVDEPLUG_OK)
	add_library(ioth_vdestack-r SHARED ioth_vdestack.c)
	set_target_properties(ioth_vdestack-r PROPERTIES PREFIX "")
	target_link_libraries(ioth_vdestack-r vdeplug)
	install(TARGETS ioth_vdestack-r DESTINATION ${SYSTEM_IOTH_PATH})
	ADD_CUSTOM_TARGET(ioth_vdestack_n.so ALL DEPENDS ioth_vdestack-r
			COMMAND ${CMAKE_COMMAND} -E create_symlink ioth_vdestack-r.so ioth_vdestack_n.so)
	install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ioth_vdestack_n.so DESTINATION ${SYSTEM_IOTH_PATH})
endif()

find_library(LIBPICOX_OK picoxnet)
check_include_file("picoxnet.h" INCLUDEPICOX_OK)
if (LIBPICOX_OK AND INCLUDEPICOX_OK)
	add_library(ioth_picox-r SHARED ioth_picox.c)
	set_target_properties(ioth_picox-r PROPERTIES PREFIX "")
	target_link_libraries(ioth_picox-r picoxnet)
	install(TARGETS ioth_picox-r DESTINATION ${SYSTEM_IOTH_PATH})
	ADD_CUSTOM_TARGET(ioth_picox_n.so ALL DEPENDS ioth_picox-r
			COMMAND ${CMAKE_COMMAND} -E create_symlink ioth_picox-r.so ioth_picox_n.so)
	install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ioth_picox_n.so DESTINATION ${SYSTEM_IOTH_PATH})
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

