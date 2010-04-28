project(setup)

configure_file(config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)

add_definitions(
    -D_FILE_OFFSET_BITS=64
)

# En mode débogage, compiler les tests
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DBUILD_TESTS)
endif (CMAKE_BUILD_TYPE STREQUAL "Debug")

include_directories(
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_SOURCE_DIR}/libpackage
        ${QT_QTCORE_INCLUDE_DIR}
        ${QT_INCLUDE_DIR}
        ${QT_QT_INCLUDE_DIR}
)

set(setup_SRCS
        cache.cpp
        main.cpp
        app.cpp
        package.cpp
        communication.cpp
        string.cpp
        source.cpp
        repoma.cpp
)

qt4_automoc(${setup_SRCS})

add_executable(setup ${setup_SRCS})

target_link_libraries(setup
        ${QT_QTCORE_LIBRARY}
        lgrpkg
)

install(TARGETS setup DESTINATION /usr/bin)
