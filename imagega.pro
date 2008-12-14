CONFIG += qt debug
TEMPLATE = app
TARGET = 
DEPENDPATH += .
INCLUDEPATH += .
SOURCES += ga.cpp
QMAKE_CXXFLAGS += -fopenmp -O2
QMAKE_LFLAGS += -fopenmp
