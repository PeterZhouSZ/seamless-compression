#include <cmath>

#include "solver.h"
#include "image.h"

#include <memory>


// -- Solver -------------------------------------------------------------------

Solver::Solver()
{

}

void Solver::fixSeams(const Mesh& m, Image& img)
{
    resx = img.resx;
    resy = img.resy;

    vi.clear();
    vi.resize(resx * resy, -1);

    sys.clear();

    vec2 uvscale(resx, resy);

    // be seamless
    for (const Seam& s : m.seam) {
        double d = m.maxLength(s, uvscale);
        for (double t = 0; t <= 1; t += 1 / (2*d)) {
            sys.addEquation(
                pixel(m.uvpos(s.first, t) * uvscale) == pixel(m.uvpos(s.second, t) * uvscale)
            );
        }
    }

    sys.printShort();
    // be yourself
    for (int y = 0; y < resy; ++y)
    for (int x = 0; x < resx; ++x) {
        if (vi[indexOf(x, y)] != -1) {
            double w = (img.mask(x, y) & Image::MaskBit::Internal) ? 1.0 : 0.1;
            //double w = 0.01;
            sys.addEquation(w * (
                pixel(x, y) == img.pixel(x, y)
            ));
        }
    }

    sys.printShort();
    std::vector<scalar> vars;
    sys.initializeVars(vars);

    double e1 = sys.squaredErrorFor(vars);
    sys.solve(vars);
    double e2 = sys.squaredErrorFor(vars);

    std::cout << "error " << e1 << " -> " << e2 << std::endl;

    for (int y = 0; y < resy; ++y)
    for (int x = 0; x < resx; ++x) {
        if (vi[indexOf(x, y)] != -1) {
            img.pixel(x, y) = glm::clamp(pixel(x, y).evaluateFor(vars), vec3(0), vec3(255));
        }
    }
}

void Solver::fixSeamsSeparateChannels(const Mesh& m, Image& img, double alpha)
{
    resx = img.resx;
    resy = img.resy;

    vec2 uvscale(resx, resy);

    assert(alpha >= 0);
    assert(alpha <= 1);

    double err_seamless = 0;
    double err_id = 0;

    for (int channel = 0; channel < 3; ++channel) {
        std::cout << "Solving for channel " << channel << std::endl;

        vi.clear();
        vi.resize(resx * resy, -1);

        sys.clear();

        // be seamless
        for (const Seam& s : m.seam) {
            double d = m.maxLength(s, uvscale);
            for (double t = 0; t <= 1; t += 1 / (2*d)) {
                sys.addEquation(
                    alpha * (pixelExp(m.uvpos(s.first, t) * uvscale) == pixelExp(m.uvpos(s.second, t) * uvscale)), "seamless"
                );
            }
        }

        // be yourself
        for (int y = 0; y < resy; ++y)
        for (int x = 0; x < resx; ++x) {
            if (vi[indexOf(x, y)] != -1) {
                double w = (img.mask(x, y) & Image::MaskBit::Internal) ? 1.0 : 0.1;
                //double w = 0.01;
                sys.addEquation(
                    (1 - alpha) * (w * (pixelExp(x, y) == img.pixel(x, y)[channel])), "id"
                );
            }
        }

        sys.printShort();

        std::vector<scalar> vars;
        sys.initializeVars(vars);

        sys.solve(vars);

        err_seamless += sys.squaredErrorFor(vars, "seamless");
        err_id += sys.squaredErrorFor(vars, "id");

        for (int y = 0; y < resy; ++y)
        for (int x = 0; x < resx; ++x) {
            if (vi[indexOf(x, y)] != -1) {
                img.pixel(x, y)[channel] = glm::clamp(pixelExp(x, y).evaluateFor(vars), 0.0, 255.0);
            }
        }
    }

    std::cout << "Error (seamless) = " << err_seamless << std::endl;
    std::cout << "Error (id)       = " << err_id << std::endl;
    std::cout << "Error (total)    = " << err_seamless + err_id << std::endl;
}

void Solver::fixSeamsSeparateChannels(const Mesh& m, Image& img, const std::vector<std::vector<Seam>>& vsv)
{
    resx = img.resx;
    resy = img.resy;

    vec2 uvscale(resx, resy);

    double toterr = 0;
    for (const std::vector<Seam>& sv : vsv) {
        std::cout << "Solving partition of " << sv.size() << " seams" << std::endl;
        double parterr = 0;
        for (int channel = 0; channel < 3; ++channel) {

            vi.clear();
            vi.resize(resx * resy, -1);

            sys.clear();

            // be seamless
            for (const Seam& s : sv) {
                double d = m.maxLength(s, uvscale);
                for (double t = 0; t <= 1; t += 1 / (2*d)) {
                    sys.addEquation(
                        pixelExp(m.uvpos(s.first, t) * uvscale) == pixelExp(m.uvpos(s.second, t) * uvscale)
                    );
                }
            }

            sys.printShort();

            // be yourself
            for (int y = 0; y < resy; ++y)
            for (int x = 0; x < resx; ++x) {
                if (vi[indexOf(x, y)] != -1) {
                    double w = (img.mask(x, y) & Image::MaskBit::Internal) ? 1.0 : 0.1;
                    //double w = 0.01;
                    sys.addEquation(w * (
                        pixelExp(x, y) == img.pixel(x, y)[channel]
                    ));
                }
            }

            sys.printShort();

            std::vector<scalar> vars;
            sys.initializeVars(vars);

            sys.solve(vars);
            double err = sys.squaredErrorFor(vars);
            parterr += err;

            for (int y = 0; y < resy; ++y)
            for (int x = 0; x < resx; ++x) {
                if (vi[indexOf(x, y)] != -1) {
                    img.pixel(x, y)[channel] = glm::clamp(pixelExp(x, y).evaluateFor(vars), 0.0, 255.0);
                }
            }
        }

        std::cout << "  Partition error = " << parterr << std::endl;
        toterr += parterr;
    }
    std::cout << "Total error = " << toterr << std::endl;
}


int Solver::indexOf(int x, int y) const
{
    x = (x + resx) % resx;
    y = (y + resy) % resy;
    return y * resx + x;
}

LinearVec3 Solver::pixel(int x, int y)
{
    int i = indexOf(x, y);
    if (vi[i] == -1) {
        vi[i] = sys.nvar;
        return sys.newLinearVec3();
    } else {
        return LinearVec3(variable(vi[i]), variable(vi[i] + 1), variable(vi[i] + 2));
    }
}

LinearVec3 Solver::pixel(vec2 p)
{
    p -= vec2(0.5);
    vec2 p0 = floor(p);
    vec2 p1 = floor(p + vec2(1));
    vec2 w = fract(p);
    return mix(
        mix(pixel(int(p0.x), int(p0.y)), pixel(int(p1.x), int(p0.y)), scalar(w.x)),
        mix(pixel(int(p0.x), int(p1.y)), pixel(int(p1.x), int(p1.y)), scalar(w.x)),
        scalar(w.y)
    );
}

LinearExp Solver::pixelExp(vec2 p)
{
    p -= vec2(0.5);
    vec2 p0 = floor(p);
    vec2 p1 = floor(p + vec2(1));
    vec2 w = fract(p);
    return mix(
        mix(pixelExp(int(p0.x), int(p0.y)), pixelExp(int(p1.x), int(p0.y)), scalar(w.x)),
        mix(pixelExp(int(p0.x), int(p1.y)), pixelExp(int(p1.x), int(p1.y)), scalar(w.x)),
        scalar(w.y)
    );
}

LinearExp Solver::pixelExp(int x, int y)
{
    int i = indexOf(x, y);
    if (vi[i] == -1) {
        vi[i] = sys.nvar;
        return sys.newVar();
    } else {
        return variable(vi[i]);
    }
}


// -- SolverCompressedImage ----------------------------------------------------

SolverCompressedImage::SolverCompressedImage()
    : cptr{nullptr}
{

}

void SolverCompressedImage::fixSeamsSeparateChannels(const Mesh& m, const Image& img, CompressedImage& cimg, double alpha)
{
    resx = img.resx;
    resy = img.resy;

    vec2 uvscale(resx, resy);

    assert(alpha >= 0);
    assert(alpha <= 1);

    cptr = &cimg;

    double err_seamless = 0;
    double err_id = 0;

    for (int channel = 0; channel < 3; ++channel) {
        std::cout << "Solving for channel " << channel << std::endl;

        vi.clear();
        vi.resize(cimg.nblk() * 2, -1);

        sys.clear();


        // be seamless
        for (const Seam& s : m.seam) {
            double d = m.maxLength(s, uvscale);
            for (double t = 0; t <= 1; t += 1 / (2*d)) {
                sys.addEquation(alpha * (pixelExp(m.uvpos(s.first, t) * uvscale) == pixelExp(m.uvpos(s.second, t) * uvscale)), "seamless");
            }
        }

        // be yourself
        for (int y = 0; y < resy; ++y)
        for (int x = 0; x < resx; ++x) {
            int bx = x / 4;
            int by = y / 4;
            if ((vi[indexOf(bx, by, 0)] != -1) || (vi[indexOf(bx, by, 1)] != -1)) {
                double w = (img.mask(x, y) & Image::MaskBit::Internal) ? 1 : 0.1;
                sys.addEquation(alpha * (w * (pixelExp(x, y) == img.pixel(x, y)[channel])), "id");
            }
        }

        sys.printShort();

        std::vector<scalar> vars;
        sys.initializeVars(vars);

        sys.solve(vars);

        err_seamless += sys.squaredErrorFor(vars, "seamless");
        err_id += sys.squaredErrorFor(vars, "id");

        for (int by = 0; by < resy/4; ++by)
        for (int bx = 0; bx < resx/4; ++bx) {
            int i0 = vi[indexOf(bx, by, 0)];
            int i1 = vi[indexOf(bx, by, 1)];
            int bi = cimg.getBlockIndex(bx * 4, by * 4);
            if (i0 != -1) {
                double c0val = glm::clamp(LinearExp(i0).evaluateFor(vars), 0.0, 255.0);
                cimg.getBlock(bi).c0[channel] = c0val;
            }
            if (i1 != -1) {
                double c1val = glm::clamp(LinearExp(i1).evaluateFor(vars), 0.0, 255.0);
                cimg.getBlock(bi).c1[channel] = c1val;
            }

        }
    }

    std::cout << "Error (seamless) = " << err_seamless << std::endl;
    std::cout << "Error (id)       = " << err_id << std::endl;
    std::cout << "Error (total)    = " << err_seamless + err_id << std::endl;

    cptr = nullptr;
}


int SolverCompressedImage::indexOf(int bx, int by, int ci) const
{
    return (by * (resx / 4) + bx) * 2 + ci;
}

LinearVec3 SolverCompressedImage::pixel(vec2 p)
{
    p -= vec2(0.5);
    vec2 p0 = floor(p);
    vec2 p1 = floor(p + vec2(1));
    vec2 w = fract(p);
    return mix(
        mix(pixel(int(p0.x), int(p0.y)), pixel(int(p1.x), int(p0.y)), scalar(w.x)),
        mix(pixel(int(p0.x), int(p1.y)), pixel(int(p1.x), int(p1.y)), scalar(w.x)),
        scalar(w.y)
    );
}

LinearVec3 SolverCompressedImage::blockVars(int bx, int by, int ci)
{
    int i = indexOf(bx, by, ci);
    if (vi[i] == -1) {
        vi[i] = sys.nvar;
        return sys.newLinearVec3();
    } else {
        return LinearVec3(variable(vi[i] + 0), variable(vi[i] + 1), variable(vi[i] + 2));
    }
}

LinearVec3 SolverCompressedImage::pixel(int x, int y)
{
    unsigned char bitmask = cptr->getMask(x, y);

    x = x / 4;
    y = y / 4;

    switch (bitmask) {
    case QMASK_C0:
        return blockVars(x, y, 0);
    case QMASK_C0_23_C1_13:
        return blockVars(x, y, 0) * (2.0 / 3.0)
             + blockVars(x, y, 1) * (1.0 / 3.0);
    case QMASK_C0_13_C1_23:
        return blockVars(x, y, 1) * (2.0 / 3.0)
             + blockVars(x, y, 0) * (1.0 / 3.0);
    case QMASK_C1:
        return blockVars(x, y, 1);
    default:
        assert(0 && "SolverCompressedImage: invalid bit mask");
    }
}

LinearExp SolverCompressedImage::pixelExp(vec2 p)
{
    p -= vec2(0.5);
    vec2 p0 = floor(p);
    vec2 p1 = floor(p + vec2(1));
    vec2 w = fract(p);
    return mix(
        mix(pixelExp(int(p0.x), int(p0.y)), pixelExp(int(p1.x), int(p0.y)), scalar(w.x)),
        mix(pixelExp(int(p0.x), int(p1.y)), pixelExp(int(p1.x), int(p1.y)), scalar(w.x)),
        scalar(w.y)
    );
}

LinearExp SolverCompressedImage::blockVarsExp(int bx, int by, int ci)
{
    int i = indexOf(bx, by, ci);
    if (vi[i] == -1) {
        vi[i] = sys.nvar;
        return sys.newVar();
    } else {
        return variable(vi[i]);
    }
}

LinearExp SolverCompressedImage::pixelExp(int x, int y)
{
    unsigned char bitmask = cptr->getMask(x, y);

    x = x / 4;
    y = y / 4;

    switch (bitmask) {
    case QMASK_C0:
        return blockVarsExp(x, y, 0);
    case QMASK_C0_23_C1_13:
        return blockVarsExp(x, y, 0) * (2.0 / 3.0)
             + blockVarsExp(x, y, 1) * (1.0 / 3.0);
    case QMASK_C0_13_C1_23:
        return blockVarsExp(x, y, 1) * (2.0 / 3.0)
             + blockVarsExp(x, y, 0) * (1.0 / 3.0);
    case QMASK_C1:
        return blockVarsExp(x, y, 1);
    default:
        assert(0 && "SolverCompressedImage: invalid bit mask");
    }
}


