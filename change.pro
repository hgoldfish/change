#-------------------------------------------------
#
# Project created by QtCreator 2015-04-19T15:25:47
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = change
TEMPLATE = app
CONFIG += c++11
TRANSLATIONS += change.ts

SOURCES += main.cpp\
        changeitwidget.cpp

HEADERS  += changeitwidget.h

FORMS    += changeitwidget.ui

RESOURCES += \
    change.qrc
