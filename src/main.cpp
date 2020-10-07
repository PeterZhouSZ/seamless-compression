#include <iostream>

#include "image.h"
#include "mesh.h"
#include "solver.h"
#include "compressed_image.h"
#include "pyramid.h"
#include "metric.h"

#include <set>
#include <algorithm>
#include <chrono>

CompressedImage compressAndOptimzeTexture(Mesh& m, const Image& texture, int maxIter)
{
    //Image img = texture;
    //img.setMaskInternal(m);
    //img.setMaskBilinear(m);

    // fix seams in R3
    //Solver().fixSeams(m, img);

    //img.save("data/seamless_R3.png");

    CompressedImage cimg;

    cimg.initialize(texture, Image::MaskBit::Seam | Image::MaskBit::Internal);
    //cimg.saveUncompressed("data/seamless_blocks.png");
    cimg.quantizeBlocks();
    //cimg.saveUncompressed("data/seamless_compressed.png");

    std::set<int> fixedBlocks;
    int n = 0;

    do {
        SolverCompressedImage().fixSeams(m, texture, cimg, fixedBlocks);
        cimg.quantizeBlocks();
        n++;

        if (n >= maxIter)
            break;

        //std::string path = "data/seamless_compressed_optimized" + std::to_string(n) + ".png";
        //cimg.saveUncompressed(path.c_str());

        std::vector<BlockErrorData> err = cimg.computePerBlockError(texture);

        std::sort(err.begin(), err.end(), [](const BlockErrorData& e1, const BlockErrorData& e2) { return e1.avgError < e2.avgError; });

        int limit = std::max(int(0.01 * err.size()), 1);
        int numInserted = 0;
        for (unsigned i = 0; i < err.size(); ++i) {
            if (fixedBlocks.insert(err[i].blkIndex).second == true) {
                numInserted++;
            }
            if (numInserted > limit)
                break;
        }

        if (numInserted == 0)
            break;

    } while (true);

    return cimg;
}

static void parseArgs(int argc, char *argv[], std::vector<std::string>& positionalArgs, std::set<char>& options)
{
    positionalArgs.clear();
    options.clear();
    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        if (arg[0] == '-' && arg.size() == 2) {
            options.insert(arg[1]);
            std::cout << "Found option: " << arg[1] << std::endl;
        } else if (arg[0] != '-') {
            positionalArgs.push_back(arg);
            std::cout << "Found positional argument: " << positionalArgs.back() << std::endl;
        } else {
            std::cerr << "Warining: unrecognized argument " << arg << std::endl;
        }
    }
}

#if 0
int main(int argc, char *argv[])
{
    std::vector<std::string> positionalArgs;
    std::set<char> options;

    parseArgs(argc, argv, positionalArgs, options);

    if (positionalArgs.size() < 2) {
        std::cerr << "Usage: " << argv[0] << " obj texture [-c]" << std::endl;
        std::exit(-1);
    }

    auto n1 = positionalArgs[0].find_last_of('/');
    if (n1 == std::string::npos)
        n1 = 0;
    else
        n1++;
    auto n2 = positionalArgs[0].find_last_of('.');
    std::string meshName = positionalArgs[0].substr(n1, n2-n1);

    Mesh m;
    m.loadObjFile(positionalArgs[0].c_str());
    m.computeSeams();

    m.mirrorV();

    Image img;
    img.load(positionalArgs[1].c_str());

    img.setMaskInternal(m);
    img.setMaskBilinear(m);

    std::string maskName = meshName + "_mask.png";
    img.saveMask(maskName.c_str());

    {
        auto t0 = std::chrono::high_resolution_clock::now();
        Solver().fixSeams(m, img);
        auto t1 = std::chrono::high_resolution_clock::now();
        std::cout << "Optimization took " << std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count() << " ms" << std::endl;

        std::string textureOutName = meshName + "_s.png";
        std::string meshOutName = meshName + "_s";
        img.save(textureOutName.c_str());
        auto t2 = std::chrono::high_resolution_clock::now();
        std::cout << "Saving texture took " << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count() << " ms" << std::endl;

        m.mirrorV();
        m.saveObjFile(meshOutName.c_str(), textureOutName.c_str());
        auto t3 = std::chrono::high_resolution_clock::now();
        std::cout << "Saving mesh took " << std::chrono::duration_cast<std::chrono::milliseconds>(t3 - t2).count() << " ms" << std::endl;

        m.mirrorV();
    }

    bool compressed = (options.find('c') != options.end());

    if (compressed) {

        {
            std::cout << "Compressing seamless texture with libsquish... " << std::endl;
            Image sc;
            CompressWithSquish(img, sc);
            std::cout << " done." << std::endl;
            std::string squishTextureName = meshName + "_sc.png";
            std::string squishMeshName = meshName + "_sc";
            sc.save(squishTextureName.c_str());
            m.mirrorV();
            m.saveObjFile(squishMeshName.c_str(), squishTextureName.c_str());
            m.mirrorV();
        }

        CompressedImage cimg = compressAndOptimzeTexture(m, img, 5);
        std::string textureOutName = meshName + "_sc_optimized.png";
        std::string meshOutName = meshName + "_sc_optimized";
        cimg.saveUncompressed(textureOutName.c_str());
        m.mirrorV();
        m.saveObjFile(meshOutName.c_str(), textureOutName.c_str());
    }

    return 0;
}
#endif

int main(int argc, char *argv[])
{
    std::vector<std::string> positionalArgs;
    std::set<char> options;

    parseArgs(argc, argv, positionalArgs, options);

    if (positionalArgs.size() < 2) {
        std::cerr << "Usage: " << argv[0] << " obj texture [-c]" << std::endl;
        std::exit(-1);
    }

    auto n1 = positionalArgs[0].find_last_of('/');
    if (n1 == std::string::npos)
        n1 = 0;
    else
        n1++;
    auto n2 = positionalArgs[0].find_last_of('.');
    std::string meshName = positionalArgs[0].substr(n1, n2-n1);

    std::cout << "Loading mesh..." << std::endl;

    Mesh m;
    m.loadObjFile(positionalArgs[0].c_str());

    std::cout << "Computing seams..." << std::endl;
    m.computeSeams();

    m.mirrorV();

    std::cout << "Loading texture..." << std::endl;
    Image img;
    img.load(positionalArgs[1].c_str());

    std::cout << "Saving source texture..." << std::endl;
    img.save("source_texture.png");

    std::cout << "Computing pixel masks..." << std::endl;

    unsigned ni = img.setMaskInternal(m);
    unsigned ns = img.setMaskSeam(m);

    std::cout << ni << " internal pixels, " << ns << " seam pixels" << std::endl;

    // save a mask image of the seam pixels
    std::string maskName = meshName + "_mask.png";
    img.saveMask(maskName.c_str(), Image::MaskBit::Seam);

    // -- seamless -------------------------------------------------------------

    Image img_seamless = img;
    {
        std::cout << "Solving seamless..." << std::endl;
        auto t0 = std::chrono::high_resolution_clock::now();
        Solver().fixSeams(m, img_seamless);
        auto t1 = std::chrono::high_resolution_clock::now();
        std::cout << "Optimization took " << std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count() << " ms" << std::endl;

        std::string textureOutName = meshName + "_s.png";
        std::string meshOutName = meshName + "_s";
        img_seamless.save(textureOutName.c_str());
        auto t2 = std::chrono::high_resolution_clock::now();
        std::cout << "Saving texture took " << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count() << " ms" << std::endl;

        m.saveObjFile(meshOutName.c_str(), textureOutName.c_str(), true);
        auto t3 = std::chrono::high_resolution_clock::now();
        std::cout << "Saving mesh took " << std::chrono::duration_cast<std::chrono::milliseconds>(t3 - t2).count() << " ms" << std::endl;
    }

    bool compressed = (options.find('c') != options.end());

    if (compressed) {

        // -- seam-aware compression 1 iteration -------------------------------
        {
            std::cout << "Solving seam-aware compression 1 iteration..." << std::endl;
            CompressedImage cimg = compressAndOptimzeTexture(m, img, 1);
            std::string textureOutName = meshName + "_sac1.png";
            std::string meshOutName = meshName + "_sac1";
            cimg.saveUncompressed(textureOutName.c_str());
            m.saveObjFile(meshOutName.c_str(), textureOutName.c_str(), true);
        }

        // -- seam-aware compression n=10 iterations ---------------------------
        /*
        {
            std::cout << "Solving seam-aware compression n iterations..." << std::endl;
            CompressedImage cimg = compressAndOptimzeTexture(m, img, 10);
            std::string textureOutName = meshName + "_sacn.png";
            std::string meshOutName = meshName + "_sacn";
            cimg.saveUncompressed(textureOutName.c_str());
            m.saveObjFile(meshOutName.c_str(), textureOutName.c_str(), true);
        }
        */

        // -- seamless seam-aware compression 1 iteration ----------------------
        {
            std::cout << "Solving seamless seam-aware compression 1 iteration..." << std::endl;
            CompressedImage cimg = compressAndOptimzeTexture(m, img_seamless, 1);
            std::string textureOutName = meshName + "_s_sac1.png";
            std::string meshOutName = meshName + "_s_sac1";
            cimg.saveUncompressed(textureOutName.c_str());
            m.saveObjFile(meshOutName.c_str(), textureOutName.c_str(), true);
        }

        // -- seamless compressed ----------------------------------------------
        {
            std::cout << "Compressing seamless texture with PCA..." << std::endl;
            CompressedImage cimg;
            cimg.initialize(img_seamless, Image::MaskBit::Internal | Image::MaskBit::Seam);
            cimg.quantizeBlocks();

            std::string textureOutName = meshName + "_sc.png";
            std::string meshOutName = meshName + "_sc";
            cimg.saveUncompressed(textureOutName.c_str());
            m.saveObjFile(meshOutName.c_str(), textureOutName.c_str(), true);

        }

        // -- seamless squish compressed ---------------------------------------
        {
            std::cout << "Compressing seamless texture with libsquish... " << std::endl;
            Image sc;
            CompressWithSquish(img_seamless, sc);
            std::cout << " done." << std::endl;
            std::string squishTextureName = meshName + "_sc_squish.png";
            std::string squishMeshName = meshName + "_sc_squish";
            sc.save(squishTextureName.c_str());
            m.saveObjFile(squishMeshName.c_str(), squishTextureName.c_str(), true);
        }
    }

    return 0;
}
