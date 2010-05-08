project(checkfiles)

include_directories(
        ${CMAKE_CURRENT_BINARY_DIR}
        ${QT_QTCORE_INCLUDE_DIR}
        ${QT_QTXML_INCLUDE_DIR}
        ${QT_INCLUDE_DIR}
        ${QT_QT_INCLUDE_DIR}
        ${CMAKE_SOURCE_DIR}/libpackage
)

set(checkfiles_SRCS
        checkfiles.cpp
)

add_definitions(
        -DQT_PLUGIN
        -DQT_NO_DEBUG
        -DQT_SHARED
        ${QT_DEFINITIONS})

qt4_automoc(${checkfiles_SRCS})
add_library(checkfiles SHARED ${checkfiles_SRCS})

target_link_libraries(checkfiles
        ${QT_QTCORE_LIBRARY}
        ${QT_QTXML_LIBRARY}
)

install(TARGETS checkfiles LIBRARY DESTINATION /usr/lib/lgrpkg)
