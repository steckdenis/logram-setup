project(shlibdeps)

find_package(ELF REQUIRED)

include_directories(
        ${CMAKE_CURRENT_BINARY_DIR}
        ${QT_QTCORE_INCLUDE_DIR}
        ${QT_QTXML_INCLUDE_DIR}
        ${QT_INCLUDE_DIR}
        ${QT_QT_INCLUDE_DIR}
        ${CMAKE_SOURCE_DIR}/libpackage
        ${ELF_INCLUDE_DIR}
)

set(shlibdeps_SRCS
        shlibdeps.cpp
)

add_definitions(
        -DQT_PLUGIN
        -DQT_NO_DEBUG
        -DQT_SHARED
        ${QT_DEFINITIONS})

qt4_automoc(${shlibdeps_SRCS})
add_library(shlibdeps SHARED ${shlibdeps_SRCS})

target_link_libraries(shlibdeps
        ${QT_QTCORE_LIBRARY}
        ${QT_QTXML_LIBRARY}
        ${ELF_LIBRARY}
)

install(TARGETS shlibdeps LIBRARY DESTINATION /usr/lib/lgrpkg)
