#ifdef __EMSCRIPTEN__

#include "mesh.h"
#include "image.h"
#include "solver.h"

#include <glm/common.hpp>

#include <emscripten.h>
#include <emscripten/bind.h>

void Image::load(uint8_t *imgbuf, int w, int h)
{
    int nc = 4;
    resize(resx, resy);
    for (int y = 0, i = 0; y < resy; ++y)
    for (int x = 0; x < resx; ++x, ++i) {
        int k = (y * resx + x) * nc;
        data[i] = vec3(imgbuf[k], imgbuf[k+1], imgbuf[k+2]);
    }
}

void Image::store(uint8_t *imgbuf, int w, int h)
{
    int nc = 4;
    assert(resx == w);
    assert(resy == h);
    for (int y = 0, i = 0; y < resy; ++y)
    for (int x = 0; x < resx; ++x, ++i) {
        int k = (y * resx + x) * nc;
        imgbuf[k] = uint8_t(glm::clamp(data[i].x, 0.0f, 255.0f));
        imgbuf[k+1] = uint8_t(glm::clamp(data[i].y, 0.0f, 255.0f));
        imgbuf[k+2] = uint8_t(glm::clamp(data[i].z, 0.0f, 255.0f));
        imgbuf[k+3] = 255;
    }
}

struct MeshObject {
    Mesh m;
    Image img;

    void loadMesh(const std::string& path);
    void smoothSeams(intptr_t imgbufptr, int w, int h);

    int fn();
    intptr_t get3DBuffer();
    intptr_t getUVBuffer();
};

int MeshObject::fn()
{
    return m.face.size();
}

intptr_t MeshObject::get3DBuffer()
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

intptr_t MeshObject::getUVBuffer()
{
    float *buf = new float[m.face.size() * 6];
    float *p = buf;
    for (auto& f : m.face) {
        for (int i = 0; i < 3; ++i) {
            *p++ = m.vtvec[f.ti[i]].x;
            *p++ = m.vtvec[f.ti[i]].y;
        }
    }
    return (intptr_t) buf;
}

void MeshObject::loadMesh(const std::string& path)
{
    m.parseObjFile(path.c_str());
    m.computeSeams();
}

void MeshObject::smoothSeams(intptr_t imgbufptr, int w, int h)
{
    img.load((uint8_t *)imgbufptr, w, h);
    m.normalizeUV(img);
    Solver().fixSeams(m, img);
    img.store((uint8_t *)imgbufptr, w, h);
}

EMSCRIPTEN_BINDINGS(MeshObject) {
    emscripten::class_<MeshObject>("MeshObject")
        .constructor<>()
        .function("loadMesh", &MeshObject::loadMesh)
        .function("smoothSeams", &MeshObject::smoothSeams)
        .function("fn", &MeshObject::fn)
        .function("get3DBuffer", &MeshObject::get3DBuffer)
        .function("getUVBuffer", &MeshObject::getUVBuffer)
    ;
}

#endif
