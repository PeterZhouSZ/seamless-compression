#ifndef BLOCK_PARTITIONER_H
#define BLOCK_PARTITIONER_H

#include <vector>
#include <set>

#include <glm/common.hpp>

#include "mesh.h"
#include "compressed_image.h"
#include "sampling.h"

class BlockPartitioner
{
    struct BlockInfo {
        int parent;
        int size;
        std::vector<Seam> seams;
    };

    int resx;
    int resy;
    std::vector<BlockInfo> blocks;

public:

    BlockPartitioner();

    void init(int xres, int yres);
    void computePartitions(const Mesh& m);
    void printSizes();

    std::vector<std::vector<Seam>> getPartitions();

private:

    int setFind(int blockIndex);
    void setUnion(int b1, int b2);
    int setUnion(const std::set<int>& iset);

};

#endif // BLOCK_PARTITIONER_H
