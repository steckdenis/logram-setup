######################################################################
# Automatically generated by qmake (2.01a) sam. sept. 5 14:16:09 2009
######################################################################

CONFIG += debug
TEMPLATE = lib
TARGET = lgrpkg
DEPENDPATH += .
INCLUDEPATH += .
QT -= gui
QT += xml network script sql
LIBS += -larchive -lgpgme -lgpg-error -lm
QMAKE_CXXFLAGS += -D_FILE_OFFSET_BITS=64 -Werror

# Input
HEADERS +=  packagesystem.h \
            databasereader.h \
            package.h \
            databasewriter.h \
            solver.h \
            packagemetadata.h \
            communication.h \
            packagelist.h \
            databasepackage.h \
            filepackage.h \
            packagesource.h \
            templatable.h \
            repositorymanager.h \
            processthread.h \
            packagecommunication.h
            
SOURCES +=  packagesystem.cpp \
            databasereader.cpp \
            package.cpp \
            databasewriter.cpp \
            solver.cpp \
            packagemetadata.cpp \
            communication.cpp \
            packagelist.cpp \
            databasepackage.cpp \
            filepackage.cpp \
            templatable.cpp \
            packagesource.cpp \
            repositorymanager.cpp \
            processthread.cpp \
            packagecommunication.cpp

includes.files = *.h
includes.path = /usr/include/logram

target.path = /usr/lib

scripts.files = *.qs
scripts.path = /etc/lgrpkg/scripts

slist.files = sources.list.sample
slist.path = /etc/lgrpkg

hscript.files = scriptapi
hscript.path = /usr/libexec

INSTALLS += includes target scripts hscript