project(pkgui)

configure_file(config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)

add_definitions(
    -D_FILE_OFFSET_BITS=64
)

include_directories(
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_SOURCE_DIR}/libpackage
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
        packageinfo.cpp
        actionlist.cpp
        progressdialog.cpp
        progresslist.cpp
        packageitem.cpp
        installwizard.cpp
        actionpage.cpp
        branchepage.cpp
        licensepage.cpp
        installpage.cpp
        donepage.cpp
        communicationdialog.cpp
)

set(pkgui_UIS
        mainwindow.ui
        flags.ui
        actionpage.ui
        branchepage.ui
        licensepage.ui
        installpage.ui
        donepage.ui
        communicationdialog.ui
)

qt4_automoc(${pkgui_SRCS})
qt4_add_resources(pkgui_resources pkgui.qrc)
qt4_wrap_ui(pkgui_ui ${pkgui_UIS})

add_executable(pkgui ${pkgui_SRCS} ${pkgui_resources} ${pkgui_ui})

target_link_libraries(pkgui
        ${QT_QTCORE_LIBRARY}
        ${QT_QTGUI_LIBRARY}
        ${QT_QTXML_LIBRARY}
        lgrpkg
)

install(TARGETS pkgui DESTINATION /usr/bin)
