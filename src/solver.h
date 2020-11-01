#ifndef SOLVER_H
#define SOLVER_H

#include "mesh.h"
#include "lineareq.h"

#include "compressed_image.h"

#include <set>

class Solver
{
    LinearEquationSet sys;

    std::vector<int> vi; // per pixel variable index

    int resx;
    int resy;

public:
    Solver();

    void fixSeams(const Mesh& m, Image& img);

    // alpha = relative weight of the seamless equations block
    void fixSeamsSeparateChannels(const Mesh& m, Image& img, double alpha);

    void fixSeamsSeparateChannels(const Mesh& m, Image& img, const std::vector<std::vector<Seam>>& vsv);

    // multi channel
    LinearVec3 pixel(int x, int y);
    LinearVec3 pixel(vec2 p); // bilinear interpolation

    // single channel
    LinearExp pixelExp(int x, int y);
    LinearExp pixelExp(vec2 p); // bilinear interpolation

    int indexOf(int x, int y) const; // TODO copy to img

};

class SolverCompressedImage {
    LinearEquationSet sys; // same as solver
    std::vector<int> vi; // same as solver
    int resx; // same as solver
    int resy; // same as solver

    CompressedImage *cptr;

public:
    SolverCompressedImage();

    void fixSeams(const Mesh& m, const Image& img, CompressedImage& cimg, const std::set<int>& fixedBlocks);

    // alpha = relative weight of the seamless equations block
    void fixSeamsSeparateChannels(const Mesh& m, const Image& img, CompressedImage& cimg, double alpha);

    int indexOf(int bx, int by, int ci) const;
    LinearVec3 pixel(int x, int y);
    LinearVec3 pixel(vec2 p); // same as Solver
    LinearVec3 blockVars(int bx, int by, int ci);


    LinearExp pixelExp(int x, int y);
    LinearExp pixelExp(vec2 p); // same as Solver
    LinearExp blockVarsExp(int bx, int by, int ci);



};

#endif // SOLVER_H
