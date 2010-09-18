project(cataloginstaller)

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

set(cataloginstaller_SRCS
        main.cpp
        app.cpp
)

qt4_automoc(${cataloginstaller_SRCS})

add_executable(cataloginstaller ${cataloginstaller_SRCS})

target_link_libraries(cataloginstaller
        ${QT_QTCORE_LIBRARY}
        ${QT_QTGUI_LIBRARY}
        ${QT_QTXML_LIBRARY}
        lgrpkg
        lgrpkgui
)

install(TARGETS cataloginstaller DESTINATION /usr/bin)
install(FILES CatalogInstaller.desktop DESTINATION /usr/share/applications)
