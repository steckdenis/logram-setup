project(pkgui)

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

set(pkgui_SRCS
        main.cpp
        mainwindow.cpp
        packagelist.cpp
        packageitem.cpp
        packageinfo.cpp
        actionlist.cpp
)

set(pkgui_UIS
        mainwindow.ui
)       

qt4_automoc(${pkgui_SRCS})
qt4_wrap_ui(pkgui_ui ${pkgui_UIS})

add_executable(pkgui ${pkgui_SRCS} ${pkgui_resources} ${pkgui_ui})

target_link_libraries(pkgui
        ${QT_QTCORE_LIBRARY}
        ${QT_QTGUI_LIBRARY}
        ${QT_QTXML_LIBRARY}
        lgrpkg
        lgrpkgui
)

install(TARGETS pkgui DESTINATION /usr/bin)
install(FILES Pkgui.desktop DESTINATION /usr/share/applications)
install(FILES pkgui.svg DESTINATION /usr/share/pixmaps)
