project(appmanager)

configure_file(config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)

add_definitions(
    -D_FILE_OFFSET_BITS=64
)

include_directories(
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_SOURCE_DIR}/libpackage
        ${CMAKE_SOURCE_DIR}/libpackageui
        ${QT_QTCORE_INCLUDE_DIR}
        ${QT_QTGUI_INCLUDE_DIR}
        ${QT_QTXML_INCLUDE_DIR}
        ${QT_INCLUDE_DIR}
        ${QT_QT_INCLUDE_DIR}
)

set(appmanager_SRCS
        main.cpp
        mainwindow.cpp
        breadcrumb.cpp
)

set(appmanager_UIS
        appmanager.ui
        entry.ui
)

qt4_automoc(${appmanager_SRCS})
qt4_wrap_ui(appmanager_uis ${appmanager_UIS})
qt4_add_resources(appmanager_resources appmanager.qrc)

add_executable(appmanager ${appmanager_SRCS} ${appmanager_resources} ${appmanager_uis})

target_link_libraries(appmanager
        ${QT_QTCORE_LIBRARY}
        ${QT_QTGUI_LIBRARY}
        ${QT_QTXML_LIBRARY}
        lgrpkg
        lgrpkgui
)

install(TARGETS appmanager DESTINATION /usr/bin)
