cmake_minimum_required(VERSION 3.29)
project(Yohane_Modifier)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_PREFIX_PATH "D:/Qt/6.8.0/mingw_64/lib/cmake")
set(app_icon_resource_windows "${CMAKE_CURRENT_SOURCE_DIR}/assets/ico.rc")
set(QRC_SOURCE_FILE resources.qrc)

find_package(
        Qt6
        COMPONENTS
        Core
        Gui
        Widgets
        REQUIRED
        Multimedia
)
add_executable(Yohane_Modifier
        main.cpp
        yohanewindow.cpp
        yohanewindow.h
        yohanewindow.ui
        ${app_icon_resource_windows}
        ${QRC_SOURCE_FILE}
)
target_link_libraries(Yohane_Modifier
        Qt::Core
        Qt::Gui
        Qt::Widgets
        Qt6::Multimedia
)
#set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES
        #WIN32_EXECUTABLE ON
        #MACOSX_BUNDLE ON
#)

if (WIN32 AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(DEBUG_SUFFIX)
    if (MSVC AND CMAKE_BUILD_TYPE MATCHES "Debug")
        set(DEBUG_SUFFIX "d")
    endif ()
    set(QT_INSTALL_PATH "${CMAKE_PREFIX_PATH}")
    if (NOT EXISTS "${QT_INSTALL_PATH}/bin")
        set(QT_INSTALL_PATH "${QT_INSTALL_PATH}/..")
        if (NOT EXISTS "${QT_INSTALL_PATH}/bin")
            set(QT_INSTALL_PATH "${QT_INSTALL_PATH}/..")
        endif ()
    endif ()
    if (EXISTS "${QT_INSTALL_PATH}/plugins/platforms/qwindows${DEBUG_SUFFIX}.dll")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/platforms/")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                "${QT_INSTALL_PATH}/plugins/platforms/qwindows${DEBUG_SUFFIX}.dll"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/platforms/")
    endif ()
    foreach (QT_LIB Core Gui Widgets)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                "${QT_INSTALL_PATH}/bin/Qt6${QT_LIB}${DEBUG_SUFFIX}.dll"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
    endforeach (QT_LIB)
endif ()
