#include "block_partitioner.h"

#include <iostream>
#include <vector>
#include <algorithm>

constexpr int CLOSED_BLOCK = -1;

BlockPartitioner::BlockPartitioner()
{

}

void BlockPartitioner::init(int resx, int resy)
{
    this->resx = resx;
    this->resy = resy;
    blocks.resize(CompressedImage::getNumberOfBlocks(resx, resy), BlockInfo());
    for (unsigned i = 0; i < blocks.size(); ++i) {
        blocks[i].parent = CLOSED_BLOCK;
        blocks[i].size = 1;
        blocks[i].seams.clear();
    }

}

void BlockPartitioner::computePartitions(const Mesh& m)
{
    using ::Seam;

    vec2 uvscale(resx, resy);
    for (const Seam& s : m.seam) {
        double d = m.maxLength(s, uvscale);
        std::set<int> ind0, ind1;
        for (double t = 0; t <= 1; t += 1 / (SEAM_SAMPLING_FACTOR*d)) {
            vec2 p0, p1, w;

            getLinearInterpolationData(m.uvpos(s.first, t) * uvscale, p0, p1, w);
            ind0.insert(CompressedImage::getBlockIndex(int(p0.x), int(p0.y), resx, resy));
            ind0.insert(CompressedImage::getBlockIndex(int(p1.x), int(p0.y), resx, resy));
            ind0.insert(CompressedImage::getBlockIndex(int(p0.x), int(p1.y), resx, resy));
            ind0.insert(CompressedImage::getBlockIndex(int(p1.x), int(p1.y), resx, resy));

            getLinearInterpolationData(m.uvpos(s.second, t) * uvscale, p0, p1, w);
            ind1.insert(CompressedImage::getBlockIndex(int(p0.x), int(p0.y), resx, resy));
            ind1.insert(CompressedImage::getBlockIndex(int(p1.x), int(p0.y), resx, resy));
            ind1.insert(CompressedImage::getBlockIndex(int(p0.x), int(p1.y), resx, resy));
            ind1.insert(CompressedImage::getBlockIndex(int(p1.x), int(p1.y), resx, resy));
        }
        assert(ind0.size() > 0);
        assert(ind1.size() > 0);

        int k0 = setUnion(ind0);
        blocks[k0].seams.push_back(s);
        int k1 = setUnion(ind1);
        setUnion(k0, k1);
    }
}

void BlockPartitioner::printSizes()
{
    std::map<int, std::vector<Seam>> partitions;
    for (int i = 0; i < blocks.size(); ++i) {
        if (blocks[i].parent != CLOSED_BLOCK) {
            int k = setFind(i);
            partitions[k].insert(partitions[k].end(), blocks[i].seams.begin(), blocks[i].seams.end());
        }
    }

    int tot_segs = 0;
    for (const auto& entry : partitions) {
        tot_segs += entry.second.size();
    }
    std::cout << "Number of partitions is " << partitions.size() << " " << blocks.size() << std::endl;
    std::cout << "Total number of segments is " << tot_segs << std::endl;


    for (int i = 0; i < blocks.size(); ++i) {
        if (blocks[i].parent == i) {
            std::cout << "Parent block at index " << i << ", set size is " << blocks[i].size << std::endl;
        }
    }
}

std::vector<std::vector<Seam>> BlockPartitioner::getPartitions()
{
    std::map<int, std::vector<int>> partitions;
    for (int i = 0; i < blocks.size(); ++i) {
        if (blocks[i].parent != CLOSED_BLOCK) {
            int k = setFind(i);
            partitions[k].push_back(i);
        }
    }

    std::vector<std::vector<Seam>> vsv;
    for (auto entry : partitions) {
        vsv.push_back(std::vector<Seam>());
        for (int k : entry.second) {
            vsv.back().insert(vsv.back().end(), blocks[k].seams.begin(), blocks[k].seams.end());
        }
    }

    std::sort(vsv.begin(), vsv.end(), [](const std::vector<Seam>& v1, const std::vector<Seam>& v2) { return v1.size() > v2.size(); });
    return vsv;
}

int BlockPartitioner::setFind(int blockIndex)
{
    if (blocks[blockIndex].parent == CLOSED_BLOCK)
        blocks[blockIndex].parent = blockIndex;

    if (blocks[blockIndex].parent != blockIndex) {
        blocks[blockIndex].parent = setFind(blocks[blockIndex].parent);
        return blocks[blockIndex].parent;
    } else {
        return blockIndex;
    }
}

void BlockPartitioner::setUnion(int b1, int b2)
{
    b1 = setFind(b1);
    b2 = setFind(b2);

    if (b1 == b2)
        return;

    if (blocks[b1].size < blocks[b2].size)
        std::swap(b1, b2);

    blocks[b2].parent = b1;
    blocks[b1].size += blocks[b2].size;
}

int BlockPartitioner::setUnion(const std::set<int>& iset)
{
    std::vector<int> iv(iset.begin(), iset.end());
    assert(iv.size() > 0);
    int k = iv.back();
    iv.pop_back();
    for (int x : iv)
        setUnion(k, x);
    return k;
}
