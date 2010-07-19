project(pkgui)

set(Boost_USE_STATIC_LIBS ON)
find_package(Boost 1.36.0 COMPONENTS regex REQUIRED)

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
        ${Boost_INCLUDE_DIRS}
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
        ${Boost_LIBRARIES}
        lgrpkg
        lgrpkgui
)

install(TARGETS pkgui DESTINATION /usr/bin)
