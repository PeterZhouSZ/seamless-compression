TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
TARGET = fixseams
#CONFIG -= qt

INCLUDEPATH += ./eigenlib

#CONFIG(release, debug|release) {
#    DEFINES += NDEBUG
#}

LIBS += -lsquish

DEFINES += SOLVER_USE_FACTORIZATION

SOURCES += \
        compressed_image.cpp \
        compressed_image_squish.cpp \
        image.cpp \
        image_io.cpp \
        line.cpp \
        lineareq_eigen.cpp \
        main.cpp \
        mesh.cpp \
        mesh_io.cpp \
        pyramid.cpp \
        solver.cpp \
        emscripten.cpp

HEADERS += \
    compressed_image.h \
    image.h \
    line.h \
    lineareq.h \
    mesh.h \
    metric.h \
    pyramid.h \
    solver.h \
    vec3.h
