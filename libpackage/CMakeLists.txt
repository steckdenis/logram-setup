project(lgrpkg)

find_package(LibArchive REQUIRED)
find_package(GPGME)

configure_file(config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)

include_directories(
        ${CMAKE_CURRENT_BINARY_DIR}
        ${QT_QTCORE_INCLUDE_DIR}
        ${QT_QTNETWORK_INCLUDE_DIR}
        ${QT_QTSCRIPT_INCLUDE_DIR}
        ${QT_QTSQL_INCLUDE_DIR}
        ${QT_QTXML_INCLUDE_DIR}
        ${QT_INCLUDE_DIR}
        ${QT_QT_INCLUDE_DIR}
        ${LIBARCHIVE_INCLUDE_DIR}
        ${GPGME_INCLUDE_DIR}
)

set(lgrpkg_SRCS
        packagesystem.cpp
        databasereader.cpp
        package.cpp
        databasewriter.cpp
        solver.cpp
        packagemetadata.cpp
        communication.cpp
        packagelist.cpp
        databasepackage.cpp
        filepackage.cpp
        templatable.cpp
        packagesource.cpp
        repositorymanager.cpp
        processthread.cpp
        packagecommunication.cpp
)

set(lgrpkg_HEADERS
        packagesystem.h
        databasereader.h
        package.h
        databasewriter.h
        solver.h
        packagemetadata.h
        communication.h
        packagelist.h
        databasepackage.h
        filepackage.h
        packagesource.h
        templatable.h
        repositorymanager.h
        processthread.h
        packagecommunication.h
)

add_definitions(
        -D_FILE_OFFSET_BITS=64 
        -D_REENTRANT 
        -DQT_SCRIPT_LIB 
        -DQT_SQL_LIB 
        -DQT_XML_LIB 
        -DQT_NETWORK_LIB 
        -DQT_CORE_LIB 
        -DQT_SHARED
        ${QT_DEFINITIONS})

qt4_automoc(${lgrpkg_SRCS})
add_library(lgrpkg SHARED ${lgrpkg_SRCS})

target_link_libraries(lgrpkg
        ${QT_QTCORE_LIBRARY}
        ${QT_QTNETWORK_LIBRARY}
        ${QT_QTSCRIPT_LIBRARY}
        ${QT_QTSQL_LIBRARY}
        ${QT_QTXML_LIBRARY}
        ${LIBARCHIVE_LIBRARY}
        ${GPGME_LIBRARY}
)

set_target_properties(lgrpkg
        PROPERTIES VERSION ${VERSION_STRING} SOVERSION ${VERSION_MAJOR}
)

install(TARGETS lgrpkg LIBRARY DESTINATION /usr/lib)
install(FILES ${lgrpkg_HEADERS} DESTINATION /usr/include)
install(FILES weight.qs DESTINATION /etc/lgrpkg/scripts)
install(FILES sources.list.sample DESTINATION /etc/lgrpkg)
install(FILES scriptapi DESTINATION /usr/libexec)

install(CODE "message(\"/etc/lgrpkg/sources.list.sample has been installed. Rename it to sources.list before using Setup. Its configuration is normally correct.\")")
