set(APPLICATION_NAME wd_regf)
project(${APPLICATION_NAME} VERSION 1.0.0 LANGUAGES CXX)
set(VERSION_CODE 1)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

IF (CMAKE_SYSTEM_NAME MATCHES "Darwin")

set(CMAKE_EXE_LINKER_FLAGS
-L/usr/local/lib
)

set(LINK_LIBS "-lregf")

ELSE()


find_package(PkgConfig REQUIRED)

pkg_check_modules(regf REQUIRED IMPORTED_TARGET libregf)
set(LINK_LIBS ${regf_LDFLAGS})


ENDIF()

set(PROJECT_SOURCES
ntregf_cmd.cpp
)

add_executable(${APPLICATION_NAME}
    ${PROJECT_SOURCES}
)

target_link_libraries(${APPLICATION_NAME} PRIVATE 
    ${LINK_LIBS}
)

install(TARGETS ${APPLICATION_NAME}
    RUNTIME DESTINATION bin
)

