add_executable(iothtest_server iothtest_server.c)
target_link_libraries(iothtest_server ioth pthread)

add_executable(iothtest_client iothtest_client.c)
target_link_libraries(iothtest_client ioth pthread)
