cmake_minimum_required(VERSION 2.6)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")

include(FindPackageHandleStandardArgs)

project(packagemanagement)

set(VERSION_MAJOR 0)
set(VERSION_MINOR 0)
set(VERSION_PATCH 70)
set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

# All what we built here depends on Qt
find_package(Qt4 REQUIRED)

# Translations
qt4_create_translation(QM_FILES libpackage libpackageui lpm buildserver pkgui i18n/libpackage_en.ts OPTIONS -locations relative -codecfortr UTF-8 -no-obsolete)

add_custom_target(translations DEPENDS ${QM_FILES})
install(FILES ${QM_FILES} DESTINATION /usr/share/qt4/translations/)

# Build everything
add_subdirectory(libpackage)
add_subdirectory(libpackageui)
add_subdirectory(lpm)
add_subdirectory(buildserver)
add_subdirectory(pkgui)
add_subdirectory(appmanager)
