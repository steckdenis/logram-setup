project(lgrpkgui)

set(Boost_USE_STATIC_LIBS ON)
find_package(Boost 1.36.0 COMPONENTS regex REQUIRED)

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
        ${Boost_INCLUDE_DIRS}
)

set(lgrpkgui_SRCS
        utils.cpp
        filterinterface.cpp
        searchbar.cpp
        categoryview.cpp
        infopane.cpp
        installwizard.cpp
        actionpage.cpp
        branchepage.cpp
        licensepage.cpp
        installpage.cpp
        donepage.cpp
        progresslist.cpp
        communicationdialog.cpp
        progressdialog.cpp
        packagedataprovider.cpp
        markdown/markdown.cpp
        markdown/markdown-tokens.cpp
)

set(lgrpkgui_UIS
        flags.ui
        actionpage.ui
        branchepage.ui
        licensepage.ui
        installpage.ui
        donepage.ui
        communicationdialog.ui
        infopane.ui
        searchbar.ui
)

set(lgrpkgui_HEADERS
        utils.h
        filterinterface.h
        searchbar.h
        categoryview.h
        infopane.h
        packagedataproviderinterface.h
        packagedataprovider.h
        installwizard.h
        progresslist.h
        communicationdialog.h
        progressdialog.h
)      

qt4_automoc(${lgrpkgui_SRCS})
qt4_add_resources(lgrpkgui_resources lgrpkgui.qrc)
qt4_wrap_ui(lgrpkgui_ui ${lgrpkgui_UIS})

add_library(lgrpkgui SHARED ${lgrpkgui_SRCS} ${lgrpkgui_resources} ${lgrpkgui_ui})

target_link_libraries(lgrpkgui
        ${QT_QTCORE_LIBRARY}
        ${QT_QTGUI_LIBRARY}
        ${QT_QTXML_LIBRARY}
        ${Boost_LIBRARIES}
        lgrpkg
)

set_target_properties(lgrpkgui
        PROPERTIES VERSION ${VERSION_STRING} SOVERSION ${VERSION_MAJOR}
) 

install(TARGETS lgrpkgui DESTINATION /usr/lib)
install(FILES ${lgrpkgui_HEADERS} DESTINATION /usr/include)
