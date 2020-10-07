#include "image.h"
#include "compressed_image.h"

#include <squish.h>

#include <chrono>
#include <iostream>

void CompressWithSquish(const Image& in, Image& out)
{
    assert(in.resx % 4 == 0);
    assert(in.resy % 4 == 0);

    out.resize(in.resx, in.resy);

    auto t0 = std::chrono::high_resolution_clock::now();
    int nb = 0;

    for (int y = 0; y < out.resy / 4; ++y)
    for (int x = 0; x < out.resx / 4; ++x) {
        std::vector<squish::u8> cblk;
        for (int h = 0; h < 4; ++h)
        for (int k = 0; k < 4; ++k) {
            vec3 c = in.pixel(4 * x + k, 4 * y + h);
            cblk.push_back(squish::u8(c.r));
            cblk.push_back(squish::u8(c.g));
            cblk.push_back(squish::u8(c.b));
            cblk.push_back(255);
        }

        CompressedBlock cb;
        squish::Compress(cblk.data(), &cb, squish::kDxt1);
        squish::Decompress(cblk.data(), &cb, squish::kDxt1);

        for (int h = 0; h < 4; ++h)
        for (int k = 0; k < 4; ++k) {
            int i = 4 * (4 * h + k);
            out.pixel(4 * x + k, 4 * y + h) = vec3(cblk[i], cblk[i+1], cblk[i+2]);
        }

        nb++;
        if ((nb % 1000) == 0) {
            auto t1 = std::chrono::high_resolution_clock::now();
            //std::cout << "Compressed " << nb << " blocks, avg time: " << (std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count() / (double) nb) << " ms" << std::endl;
        }
    }
}
