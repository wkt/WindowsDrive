cmake_minimum_required(VERSION 3.5)

set(APPLICATION_NAME WindowsDrive)
project(${APPLICATION_NAME} VERSION 1.0.5 LANGUAGES CXX)
set(VERSION_CODE 7)

set(APPLICATION_ORG "com.weiketing.software")
set(APPLICATION_ID "${APPLICATION_ORG}.${APPLICATION_NAME}")
set(COPY_RIGHT "Copyright © 2022. All rights reserved.")
set(HOME_URL "https://windows-drive.weiketing.com:5886/home" CACHE STRING "string")
set(HTTP_API_URL "https://windows-drive.weiketing.com:5886/api/v1" CACHE STRING "string")

set(SHOW_ABOUT_QT 1 CACHE STRING "int")

set(BUILD_APPIMAGE 0 CACHE STRING "int")

set(CMAKE_VERBOSE_MAKEFILE ON CACHE BOOL "BOOL")

SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g3")
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O2 -Wall")

string(TIMESTAMP BUILD_DATE_TIME "%Y-%m-%d %H:%M:%S")

configure_file(config.h.in config.h @ONLY)


include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(INCLUDE_DIRS
    ${PROJECT_SOURCE_DIR}/src
)

set(QRC_SOURCE_FILES res/app.qrc)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Network LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Network LinguistTools)


SOURCE_GROUP("Resource Files" FILES ${QRC_SOURCE_FILES})

set(TS_FILES
    res/WindowsDrive_zh_CN.ts)

SOURCE_GROUP("Translation Files" FILES ${TS_FILES})

set(PROJECT_SOURCES
        src/main.cpp
        src/mainwindow.cpp
        src/mainwindow.h
        src/mainwindow.ui
        src/ntmounteddevices.cpp
        src/ntmounteddevices.h
        src/ntregf.cpp
        src/ntregf.h
        src/utils.cpp
        src/utils.h
        src/qapp.h
        src/qapp.cpp
        src/drivebutton.h
        src/drivebutton.cpp
        src/drivebutton.ui
        src/settingsdialog.h
        src/settingsdialog.cpp
        src/settingsdialog.ui
        src/loading_dialog.h
        src/loading_dialog.cpp
        src/qsiapplication.cpp
        src/qsiapplication.h
        src/passwddialog.h
        src/passwddialog.cpp
        src/passwddialog.ui
        src/networkmanager.h
        src/networkmanager.cpp
        src/handlereply.h
        src/handlereply.cpp
        src/functhread.h
        src/functhread.cpp
        src/fuck_stupid_lupdate.cpp
        ${TS_FILES}
        ${QRC_SOURCE_FILES}
)


set(DDI_SRC
    src/wd_helper.cpp
)

# Can not handle .c file in right way,This force me rename file to .cpp
# fucking idiot cmake
add_executable(wd_helper ${DDI_SRC})

IF (CMAKE_SYSTEM_NAME MATCHES "Darwin")

set(env{MACOSX_DEPLOYMENT_TARGET} "10.9")

set(PROJECT_SOURCES
    src/mac_utils.h
    src/mac_utils.mm
    ${PROJECT_SOURCES}
 )

set(LINK_LIBS
    "${CMAKE_PREFIX_PATH}/lib/QtSvg.framework")

set(CMAKE_EXE_LINKER_FLAGS
    "-fobjc-arc -fobjc-link-runtime"
)

set(INCLUDE_DIRS
    ${INCLUDE_DIRS}
    "/usr/local/include"
)


add_custom_target(APP_RESOURCES ALL
    DEPENDS wd_helper wd_regf ${APPLICATION_NAME}
    COMMAND mkdir -p "${CMAKE_CURRENT_BINARY_DIR}/${APPLICATION_NAME}.app/Contents/Resources"
    COMMAND cp -va "${CMAKE_CURRENT_SOURCE_DIR}/res/zh.lproj" "${CMAKE_CURRENT_BINARY_DIR}/${APPLICATION_NAME}.app/Contents/Resources"
    COMMAND cp -va "${CMAKE_CURRENT_SOURCE_DIR}/res/en.lproj" "${CMAKE_CURRENT_BINARY_DIR}/${APPLICATION_NAME}.app/Contents/Resources"
    COMMAND cp -a "${CMAKE_CURRENT_SOURCE_DIR}/res/icon.icns" "${CMAKE_CURRENT_BINARY_DIR}/${APPLICATION_NAME}.app/Contents/Resources"
    COMMAND cp -a "${CMAKE_CURRENT_BINARY_DIR}/wd_helper" "${CMAKE_CURRENT_BINARY_DIR}/${APPLICATION_NAME}.app/Contents/MacOS"
)

configure_file(res/app.plist.in ${CMAKE_CURRENT_BINARY_DIR} @ONLY)

ELSE()

set(CMAKE_CXX_FLAGS " -lrt -lpthread -Wl,--copy-dt-needed-entries")

ENDIF()

include_directories(${INCLUDE_DIRS})

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(${APPLICATION_NAME}
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )

# Define target properties for Android with Qt 6 as:
#    set_property(TARGET WindowsDrive APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation

    qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
else()
    if(ANDROID)
        add_library(${APPLICATION_NAME} SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
        configure_file(res/ts.qrc ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
        add_executable(${APPLICATION_NAME}
            ${PROJECT_SOURCES}
            ${QM_FILES}
            ${CMAKE_CURRENT_BINARY_DIR}/ts.qrc
        )
    endif()

endif()
add_dependencies(${APPLICATION_NAME} wd_regf wd_helper)


target_link_libraries(${APPLICATION_NAME} PRIVATE 
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
    ${LINK_LIBS}
    pthread)

set_target_properties(${APPLICATION_NAME} PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_BINARY_DIR}/app.plist.in
    MACOSX_BUNDLE_GUI_IDENTIFIER ${APPLICATION_ID}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_BUNDLE_NAME ${APPLICATION_NAME}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE_COPYRIGHT ${COPY_RIGHT}
    MACOSX_BUNDLE_ICON_FILE "icon.icns"
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)


if (CMAKE_SYSTEM_NAME MATCHES "Linux")

install(TARGETS
    wd_helper
    ${APPLICATION_NAME}
    RUNTIME DESTINATION bin
)

install(FILES res/windows-drive.svg
    DESTINATION share/icons/hicolor/scalable/apps
)

install(FILES res/windows-drive.desktop
    DESTINATION share/applications
)

endif()


if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(${APPLICATION_NAME})
endif()

IF(NOT (CMAKE_BUILD_TYPE MATCHES "Release"))
subdirs(tests)
ENDIF()

add_subdirectory(src/ntregf)

IF (CMAKE_SYSTEM_NAME MATCHES "Darwin")

add_custom_target(ntregf_APP_RESOURCES ALL
    DEPENDS wd_regf ${APPLICATION_NAME}
    COMMAND cp -a "${CMAKE_CURRENT_BINARY_DIR}/src/ntregf/wd_regf" "${CMAKE_CURRENT_BINARY_DIR}/${APPLICATION_NAME}.app/Contents/MacOS"
)

ENDIF()
