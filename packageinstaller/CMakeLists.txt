project(packageinstaller)

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

set(packageinstaller_SRCS
        main.cpp
        mainwindow.cpp
)

qt4_automoc(${packageinstaller_SRCS})

add_executable(packageinstaller ${packageinstaller_SRCS} ${packageinstaller_resources})

target_link_libraries(packageinstaller
        ${QT_QTCORE_LIBRARY}
        ${QT_QTGUI_LIBRARY}
        ${QT_QTXML_LIBRARY}
        lgrpkg
        lgrpkgui
)

install(TARGETS packageinstaller DESTINATION /usr/bin)
