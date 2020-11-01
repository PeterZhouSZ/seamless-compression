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
        block_partitioner.cpp \
        compress_squish.cpp \
        compressed_image.cpp \
        image.cpp \
        image_io.cpp \
        line.cpp \
        lineareq_eigen.cpp \
        main.cpp \
        mesh.cpp \
        mesh_io.cpp \
        solver.cpp \
        emscripten.cpp

HEADERS += \
    block_partitioner.h \
    compress_squish.h \
    compressed_image.h \
    image.h \
    line.h \
    lineareq.h \
    mesh.h \
    metric.h \
    sampling.h \
    solver.h \
    vec3.h
