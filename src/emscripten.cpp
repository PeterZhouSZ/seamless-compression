#ifdef __EMSCRIPTEN__

#include "mesh.h"
#include "image.h"
#include "solver.h"

#include <glm/common.hpp>

#include <emscripten.h>
#include <emscripten/bind.h>

void Image::read(uint8_t *imgbuf, int w, int h)
{
    int nc = 4;
    resize(w, h);
    for (int y = 0, i = 0; y < resy; ++y)
    for (int x = 0; x < resx; ++x, ++i) {
        int k = (y * resx + x) * nc;
        data[i] = vec3(imgbuf[k], imgbuf[k+1], imgbuf[k+2]);
    }
}

void Image::write(uint8_t *imgbuf)
{
    int nc = 4;
    for (int y = 0, i = 0; y < resy; ++y)
    for (int x = 0; x < resx; ++x, ++i) {
        int k = (y * resx + x) * nc;
        imgbuf[k] = uint8_t(glm::clamp(data[i].x, 0.0f, 255.0f));
        imgbuf[k+1] = uint8_t(glm::clamp(data[i].y, 0.0f, 255.0f));
        imgbuf[k+2] = uint8_t(glm::clamp(data[i].z, 0.0f, 255.0f));
        imgbuf[k+3] = 255;
    }
}

void CompressedImage::writeAsRGB(uint8_t *imgbuf) const
{
    int nc = 4;
    for (int y = 0; y < resy; ++y)
    for (int x = 0; x < resx; ++x) {
        int k = (y * resx + x) * nc;
        vec3 p = pixel(x, y);
        imgbuf[k] = uint8_t(glm::clamp(p.x, 0.0f, 255.0f));
        imgbuf[k+1] = uint8_t(glm::clamp(p.y, 0.0f, 255.0f));
        imgbuf[k+2] = uint8_t(glm::clamp(p.z, 0.0f, 255.0f));
        imgbuf[k+3] = 255;
    }
}

struct ProcessingInterface {

    Mesh m;
    int resx;
    int resy;

    int isz;
    uint8_t *imgbuf;

    uint8_t *outputbuf;

    int csz;
    uint8_t *compressedbuf;

    ProcessingInterface();

    void allocateImageBuffers(int w, int h);

    emscripten::val getImageBuf();
    emscripten::val getOutputBuf();
    emscripten::val getCompressedBuf();

    void loadMesh(const std::string& path);

    void compress();
    void smooth(double alpha);
    void compressAndSmooth(double alpha);

    int fn();
    intptr_t get3DBuffer();
    intptr_t getUVBufferMirrorV();
};

int ProcessingInterface::fn()
{
    return m.face.size();
}

intptr_t ProcessingInterface::get3DBuffer()
{
    float *buf = new float[m.face.size() * 9];
    float *p = buf;
    for (auto& f : m.face) {
        for (int i = 0; i < 3; ++i) {
            *p++ = m.vvec[f.pi[i]].x;
            *p++ = m.vvec[f.pi[i]].y;
            *p++ = m.vvec[f.pi[i]].z;
        }
    }
    return (intptr_t) buf;
}

intptr_t ProcessingInterface::getUVBufferMirrorV()
{
    float *buf = new float[m.face.size() * 6];
    float *p = buf;
    for (auto& f : m.face) {
        for (int i = 0; i < 3; ++i) {
            *p++ = m.vtvec[f.ti[i]].x;
            *p++ = 1 - m.vtvec[f.ti[i]].y;
        }
    }
    return (intptr_t) buf;
}

ProcessingInterface::ProcessingInterface()
    : m(), resx(0), resy(0), isz(0), imgbuf(nullptr), outputbuf(nullptr), csz(0), compressedbuf(nullptr)
{
}

void ProcessingInterface::allocateImageBuffers(int w, int h)
{
    resx = w;
    resy = h;

    if (imgbuf) {
        delete [] imgbuf;
        imgbuf = nullptr;
    }

    if (outputbuf) {
        delete [] outputbuf;
        outputbuf = nullptr;
    }

    isz = resx * resy * 4;
    imgbuf = new uint8_t[isz];
    outputbuf = new uint8_t[isz];
}

emscripten::val ProcessingInterface::getImageBuf()
{
    if (imgbuf)
        return emscripten::val(emscripten::typed_memory_view(isz, imgbuf));
    else
        return emscripten::val(0);
}

emscripten::val ProcessingInterface::getOutputBuf()
{
    if (outputbuf)
        return emscripten::val(emscripten::typed_memory_view(isz, outputbuf));
    else
        return emscripten::val(0);
}

emscripten::val ProcessingInterface::getCompressedBuf()
{
    if (compressedbuf)
        return emscripten::val(emscripten::typed_memory_view(csz, compressedbuf));
    else
        return emscripten::val(0);
}

void ProcessingInterface::loadMesh(const std::string& path)
{
    m.loadObjFile(path.c_str());
    m.computeSeams();
    m.mirrorV();
}

void ProcessingInterface::smooth(double alpha)
{
    Image img;
    img.read(imgbuf, resx, resy);

    unsigned ni = img.setMaskInternal(m);
    unsigned ns = img.setMaskSeam(m);

    Solver().fixSeamsSeparateChannels(m, img, alpha);

    img.write(outputbuf);
}

void ProcessingInterface::compress()
{
    CompressedImage cimg;
    Image img;
    img.read(imgbuf, resx, resy);

    unsigned ni = img.setMaskInternal(m);
    unsigned ns = img.setMaskSeam(m);

    cimg.initialize(img, Image::MaskBit::Internal | Image::MaskBit::Seam);
    cimg.quantizeBlocks();

    csz = cimg.write(&compressedbuf);
    cimg.writeAsRGB(outputbuf);
}

void ProcessingInterface::compressAndSmooth(double alpha)
{
    smooth(alpha);

    Image seamless;
    seamless.read(outputbuf, resx, resy);
    unsigned ni = seamless.setMaskInternal(m);
    unsigned ns = seamless.setMaskSeam(m);

    CompressedImage cimg;
    cimg.initialize(seamless, Image::MaskBit::Internal | Image::MaskBit::Seam);
    SolverCompressedImage().fixSeamsSeparateChannels(m, seamless, cimg, alpha);
    cimg.quantizeBlocks();

    csz = cimg.write(&compressedbuf);
    cimg.writeAsRGB(outputbuf);
}

EMSCRIPTEN_BINDINGS(processing_interface) {
    emscripten::class_<ProcessingInterface>("ProcessingInterface")
        .constructor<>()
        .function("allocateImageBuffers", &ProcessingInterface::allocateImageBuffers)
        .function("getImageBuf"         , &ProcessingInterface::getImageBuf)
        .function("getOutputBuf"        , &ProcessingInterface::getOutputBuf)
        .function("getCompressedBuf"    , &ProcessingInterface::getCompressedBuf)
        .function("loadMesh"            , &ProcessingInterface::loadMesh)
        .function("compress"            , &ProcessingInterface::compress)
        .function("smooth"              , &ProcessingInterface::smooth)
        .function("compressAndSmooth"   , &ProcessingInterface::compressAndSmooth)
        .function("fn"                  , &ProcessingInterface::fn)
        .function("get3DBuffer"         , &ProcessingInterface::get3DBuffer)
        .function("getUVBufferMirrorV"  , &ProcessingInterface::getUVBufferMirrorV)
    ;
}

#endif
