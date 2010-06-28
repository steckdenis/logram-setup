project(buildserver)

configure_file(config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)

add_definitions(
    -D_FILE_OFFSET_BITS=64
)

include_directories(
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_SOURCE_DIR}/libpackage
        ${QT_QTCORE_INCLUDE_DIR}
        ${QT_QTSQL_INCLUDE_DIR}
        ${QT_QTXML_INCLUDE_DIR}
        ${QT_QTNETWORK_INCLUDE_DIR}
        ${QT_INCLUDE_DIR}
        ${QT_QT_INCLUDE_DIR}
)

set(buildserver_SRCS
        main.cpp
        app.cpp
        thread.cpp
        worker.cpp
        smtp.cpp
)

qt4_automoc(${buildserver_SRCS})
qt4_add_resources(buildserver_resources buildserver.qrc)

add_executable(buildserver ${buildserver_SRCS} ${buildserver_resources})

target_link_libraries(buildserver
        ${QT_QTCORE_LIBRARY}
        ${QT_QTXML_LIBRARY}
        ${QT_QTSQL_LIBRARY}
        ${QT_QTNETWORK_LIBRARY}
        lgrpkg
)

install(TARGETS buildserver DESTINATION /usr/bin)
install(FILES buildserver.conf.sample DESTINATION /etc)
install(FILES cleanup mkskel DESTINATION /usr/bin/lgrpkg PERMISSIONS OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

install(CODE "message(\"The files cleanup and mkskel are not installed, but they are useful for managing the server. They are in buildserver/ . Also remember to copy /etc/buildserver.conf.sample to buildserver.conf, and edit it.\")")
