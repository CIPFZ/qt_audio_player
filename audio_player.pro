QT += core gui widgets quick quickcontrols2 concurrent
CONFIG += c++17 link_pkgconfig
TARGET = audio_player
TEMPLATE = app
PKGCONFIG += libavformat libavcodec libavutil libswresample portaudio-2.0
SOURCES += main.cpp librarycontroller.cpp libraryimporter.cpp \
           audiodecoder.cpp audioplayer.cpp playbackworker.cpp playercontroller.cpp \
           playlistmodel.cpp playliststore.cpp
HEADERS += librarycontroller.h libraryimporter.h \
           audiodecoder.h audioplayer.h pcmbuffer.h playbackworker.h playercontroller.h \
           playlistmodel.h playliststore.h
RESOURCES += resources.qrc
win32-msvc*: QMAKE_CXXFLAGS += /utf-8
