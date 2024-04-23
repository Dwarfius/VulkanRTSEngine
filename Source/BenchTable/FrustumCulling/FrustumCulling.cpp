#include "Precomp.h"
#include <smmintrin.h>
#include <random>

static void Init(glm::vec4& aPos, float& aDist, glm::vec4 (&aPlanes)[6])
{
    std::mt19937 generator(1234);
    std::uniform_real_distribution<float> dist;
    aPos.x = dist(generator);
    aPos.y = dist(generator);
    aPos.z = dist(generator);
    aPos.w = dist(generator);

    aDist = dist(generator);

    for (char i = 0; i < 6; i++)
    {
        glm::vec4& plane = aPlanes[i];
        plane.x = dist(generator);
        plane.y = dist(generator);
        plane.z = dist(generator);
        plane.w = dist(generator);
    }
}

// taken from Camera.h
bool Cull(glm::vec4 aPos, float aDist, glm::vec4 aPlanes[6])
{
    for(char i=0; i<6; i++)
    {
        if(glm::dot(aPos, aPlanes[i]) + aDist < 0.f)
        {
            return false;
        }
    }
    return true;
}

void TestCull(benchmark::State& aState)
{
    glm::vec4 pos;
    float dist;
    glm::vec4 planes[6];
    Init(pos, dist, planes);
    for (auto _ : aState)
    {
        benchmark::DoNotOptimize(Cull(pos, dist, planes));
    }
}

BENCHMARK(TestCull);

bool SimdCull(glm::vec4 aPos, float aDist, glm::vec4 aPlanes[6])
{
    __m128 planesSimd[6];
    for(char i=0; i<6; i++)
    {
        planesSimd[i] = _mm_load_ps(&aPlanes[i].x);
    }

    __m128 posSimd{_mm_load_ps(&aPos.x)};

    __m128 dot0 = _mm_dp_ps(posSimd, planesSimd[0], 0b11110000);
    __m128 dot1 = _mm_dp_ps(posSimd, planesSimd[1], 0b11110000);
    __m128 dot0100 = _mm_blend_ps(dot0, dot1, 0b0010);
    __m128 dot2 = _mm_dp_ps(posSimd, planesSimd[2], 0b11110000);
    __m128 dot3 = _mm_dp_ps(posSimd, planesSimd[3], 0b11110000);
    __m128 dot2223 = _mm_blend_ps(dot2, dot3, 0b1000);
    __m128 dot0123 = _mm_blend_ps(dot0100, dot2223, 0b1100);

    __m128 dot4 = _mm_dp_ps(posSimd, planesSimd[4], 0b11110000);
    __m128 dot5 = _mm_dp_ps(posSimd, planesSimd[5], 0b11110000);
    __m128 dot4544 = _mm_blend_ps(dot4, dot5, 0b0010);

    aDist *= -1;
    __m128 distSimd = _mm_load_ps1(&aDist);
    __m128 outside0123 = _mm_cmplt_ps(dot0123, distSimd);
    __m128 outside4544 = _mm_cmplt_ps(dot4544, distSimd);
    return _mm_movemask_ps(outside0123) + _mm_movemask_ps(outside4544) == 0;
}

void TestSimdCull(benchmark::State& aState)
{
    glm::vec4 pos;
    float dist;
    glm::vec4 planes[6];
    Init(pos, dist, planes);
    for (auto _ : aState)
    {
        benchmark::DoNotOptimize(SimdCull(pos, dist, planes));
    }
}

BENCHMARK(TestSimdCull);