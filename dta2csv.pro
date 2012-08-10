#-------------------------------------------------
#
# Project created by QtCreator 2011-02-25T12:24:03
#
#-------------------------------------------------

QT       += core

QT       -= gui

TARGET = dta2csv
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app


SOURCES += \
    dta2csv/main.cpp \
    dtafile/dtafile.cpp \
    dtafile/datafile.cpp

HEADERS += \
    dtafile/dtafile.h \
    dtafile/datafile.h

OTHER_FILES += \
    dta2csv.txt
