TEMPLATE = app

QT += core dbus
QT -= gui

include(config.pri)

CONFIG   += console
CONFIG   -= app_bundle

HEADERS += src/reverse_tunnel.h
SOURCES += src/main.cpp src/reverse_tunnel.cpp

INCLUDEPATH += 

LIBS += -L/usr/local/lib/ -L/usr/lib/

CONFIG += c++11 debug_and_release

CONFIG(debug, debug|release) {
    OBJECTS_DIR = .build/debug/obj
    MOC_DIR = .build/debug/moc
    DESTDIR = .build/debug
    TARGET = reversed
}
else {
    OBJECTS_DIR = .build/release/obj
    MOC_DIR = .build/release/moc
    DESTDIR = .build/release
    TARGET = reverse
    
    target.path = portable/
    INSTALLS += target
}
