CONFIG += qt debug
TEMPLATE = app
TARGET = 
DEPENDPATH += .
INCLUDEPATH += .
SOURCES += ga.cpp
QMAKE_CXXFLAGS += -fopenmp
QMAKE_LFLAGS += -fopenmp
