add_library(ioth_kernel-r SHARED ioth_kernel.c)
set_target_properties(ioth_kernel-r PROPERTIES PREFIX "")
install(TARGETS ioth_kernel-r DESTINATION ${SYSTEM_IOTH_PATH})
ADD_CUSTOM_TARGET(ioth_kernel_n.so ALL DEPENDS ioth_kernel-r
	COMMAND ${CMAKE_COMMAND} -E create_symlink ioth_kernel-r.so ioth_kernel_n.so)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ioth_kernel_n.so DESTINATION ${SYSTEM_IOTH_PATH})

add_library(ioth_vdestack-r SHARED ioth_vdestack.c)
set_target_properties(ioth_vdestack-r PROPERTIES PREFIX "")
target_link_libraries(ioth_vdestack-r vdeplug)
install(TARGETS ioth_vdestack-r DESTINATION ${SYSTEM_IOTH_PATH})
ADD_CUSTOM_TARGET(ioth_vdestack_n.so ALL DEPENDS ioth_vdestack-r
	COMMAND ${CMAKE_COMMAND} -E create_symlink ioth_vdestack-r.so ioth_vdestack_n.so)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ioth_vdestack_n.so DESTINATION ${SYSTEM_IOTH_PATH})

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
