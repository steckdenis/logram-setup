######################################################################
# Automatically generated by qmake (2.01a) sam. sept. 5 14:16:09 2009
######################################################################

CONFIG += debug
TEMPLATE = lib
TARGET = lgrpkg
DEPENDPATH += .
INCLUDEPATH += .
QT -= gui
QT += xml network script

# Input
HEADERS += libpackage.h libpackage_p.h package.h databasewriter.h solver.h
SOURCES += libpackage.cpp libpackage_p.cpp package.cpp databasewriter.cpp solver.cpp

includes.files = *.h
includes.path = /usr/include/logram

target.path = /usr/lib

scripts.files = *.qs
scripts.path = /etc/setup/scripts

INSTALLS += includes target scripts