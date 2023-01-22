#pragma once
#include <iostream>
#include <cmath>
#include <random>

#undef M_PI
#define M_PI 3.141592653589793f

#define USE_MULTI_THREAD true
const int MAX_THREAD_NUM = 20;
const int MAX_JOB_NUM = 1024;

extern const float  EPSILON;
const float kInfinity = std::numeric_limits<float>::max();
extern int BVH_SAH_N;
extern bool SAMPLE_UNIFORM;
extern int SPP;
extern float PRR;

inline float clamp(const float &lo, const float &hi, const float &v)
{ return std::max(lo, std::min(hi, v)); }

inline float lerp(const float &x, const float &y, const float &alpha)
{ return (1 - alpha) * x + alpha * y; }

inline  bool solveQuadratic(const float &a, const float &b, const float &c, float &x0, float &x1)
{
    float discr = b * b - 4 * a * c;
    if (discr < 0) return false;
    else if (discr == 0) x0 = x1 = - 0.5 * b / a;
    else {
        float q = (b > 0) ?
                  -0.5 * (b + sqrt(discr)) :
                  -0.5 * (b - sqrt(discr));
        x0 = q / a;
        x1 = c / q;
    }
    if (x0 > x1) std::swap(x0, x1);
    return true;
}

inline float get_random_float()
{
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_real_distribution<float> dist(0.f, 1.f); // distribution in range [0, 1]

    return dist(rng);
}

inline void UpdateProgress(float progress)
{
    int barWidth = 70;

    std::cout << "[";
    int pos = barWidth * progress;
    for (int i = 0; i < barWidth; ++i) {
        if (i < pos) std::cout << "=";
        else if (i == pos) std::cout << ">";
        else std::cout << " ";
    }
    std::cout << "] " << int(progress * 100.0) << " %\r";
    std::cout.flush();
};