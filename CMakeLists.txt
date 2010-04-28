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

# Build everything
add_subdirectory(libpackage)
add_subdirectory(setup)
add_subdirectory(buildserver)
