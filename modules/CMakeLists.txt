add_library(ioth_kernel-r SHARED ioth_kernel.c)
set_target_properties(ioth_kernel-r PROPERTIES PREFIX "")
install(TARGETS ioth_kernel-r DESTINATION ${SYSTEM_IOTH_PATH})
ADD_CUSTOM_TARGET(ioth_kernel_n.so ALL DEPENDS ioth_kernel-r
	COMMAND ${CMAKE_COMMAND} -E create_symlink ioth_kernel-r.so ioth_kernel_n.so)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ioth_kernel_n.so DESTINATION ${SYSTEM_IOTH_PATH})

add_library(ioth_vdestack-r SHARED ioth_vdestack.c)
set_target_properties(ioth_vdestack-r PROPERTIES PREFIX "")
target_link_libraries(ioth_vdestack-r vdeplug -lpthread)

install(TARGETS ioth_vdestack-r DESTINATION ${SYSTEM_IOTH_PATH})
ADD_CUSTOM_TARGET(ioth_vdestack_n.so ALL DEPENDS ioth_vdestack-r
	COMMAND ${CMAKE_COMMAND} -E create_symlink ioth_vdestack-r.so ioth_vdestack_n.so)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ioth_vdestack_n.so DESTINATION ${SYSTEM_IOTH_PATH})
