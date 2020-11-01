#ifndef IMAGE_H
#define IMAGE_H

#include <vector>

#include <glm/vec3.hpp>
#include <glm/vec2.hpp>

struct Mesh;

class Image
{
    std::vector<glm::vec3> data;
    //std::vector<float> mask;
    std::vector<uint8_t> mask_;

public:

    enum MaskBit {
        Internal = 1,
        Seam = 2
    };

    int resx;
    int resy;

    Image() : resx(0), resy(0) {}

#ifdef __EMSCRIPTEN__
    void read(uint8_t *imgbuf, int w, int h);
    void write(uint8_t *imgbuf);
#else
    bool load(const char *path);
    bool save(const char *path) const;
    bool saveMask(const char *path, uint8_t bits) const;
#endif

    void resize(int rx, int ry);

    void drawLine(glm::vec2 from, glm::vec2 to, glm::vec3 c);
    void drawPoint(glm::vec2 p, glm::vec3 c);

    unsigned indexOf(int x, int y) const;

    glm::vec3& pixel(int x, int y) {
        return data[indexOf(x, y)];
    }

    glm::vec3 pixel(int x, int y) const {
        return data[indexOf(x, y)];
    }

    glm::vec3 pixel(glm::vec2 p) const;

    uint8_t mask(int x, int y) const {
        return mask_[indexOf(x, y)];
    }

    uint8_t& mask(int x, int y) {
        return mask_[indexOf(x, y)];
    }

    /*
    float weight_(int x, int y) const {
        return mask[indexOf(x, y)];
    }

    float& weight_(int x, int y) {
        return mask[indexOf(x, y)];
    }
    */

    // fetches component of the texture lookup with their weights
    void fetch(glm::vec2 p, glm::vec3& t00, glm::vec3& t10, glm::vec3& t01, glm::vec3& t11,
               double &w00, double &w10, double &w01, double& w11) const;

    // fetches the pixels contributing to the bilinear interpolation lookup of p
    void fetchIndex(glm::vec2 p, glm::vec3& p00, glm::vec3& p10, glm::vec3& p01, glm::vec3& p11) const;

    void clearMask();
    unsigned setMaskInternal(const Mesh& m);
    unsigned setMaskSeam(const Mesh& m);

};

#endif // IMAGE_H
