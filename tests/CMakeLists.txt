set(TEST_SRC
    test_main.cpp
    ../src/ntmounteddevices.cpp
    ../src/ntmounteddevices.h
    ../src/ntregf.cpp
    ../src/ntregf.h
    ../src/utils.cpp
    ../src/utils.h
)
if (CMAKE_SYSTEM_NAME MATCHES "Darwin")

set(TEST_SRC
    ../src/mac_utils.h
    ../src/mac_utils.mm
    ${TEST_SRC}
 )

endif()

add_executable(test_main ${TEST_SRC})
target_link_libraries(test_main ${LINK_LIBS})
