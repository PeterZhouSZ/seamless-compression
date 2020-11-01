#ifndef SAMPLING_H
#define SAMPLING_H

#include <glm/common.hpp>

constexpr double SEAM_SAMPLING_FACTOR = 2.0;

inline void getLinearInterpolationData(glm::vec2 p, glm::vec2& p0, glm::vec2& p1, glm::vec2& w)
{
    p -= glm::vec2(0.5);
    p0 = glm::floor(p);
    p1 = floor(p + glm::vec2(1));
    w = fract(p);
}

#endif // SAMPLING_H
