#include <cstdint>

#include "hwy/base.h"
#include "hwy/highway.h"

extern "C" bool xent_highway_probe(void) {
    /* Compile-time probe only: if this translation unit builds, Highway is wired. */
    return true;
}

extern "C" float xent_highway_sum_f32(const float *values, uint32_t count) {
    if (!values || count == 0u) {
        return 0.0f;
    }

    const hwy::HWY_NAMESPACE::ScalableTag<float> d;
    const size_t lanes = hwy::HWY_NAMESPACE::Lanes(d);
    auto acc = hwy::HWY_NAMESPACE::Zero(d);

    uint32_t i = 0u;
    for (; i + (uint32_t)lanes <= count; i += (uint32_t)lanes) {
        auto v = hwy::HWY_NAMESPACE::LoadU(d, values + i);
        acc = hwy::HWY_NAMESPACE::Add(acc, v);
    }

    float sum = hwy::HWY_NAMESPACE::GetLane(hwy::HWY_NAMESPACE::SumOfLanes(d, acc));
    for (; i < count; ++i) {
        sum += values[i];
    }
    return sum;
}

extern "C" void xent_highway_fill_f32(float *values, uint32_t count, float value) {
    if (!values || count == 0u) {
        return;
    }

    const hwy::HWY_NAMESPACE::ScalableTag<float> d;
    const size_t lanes = hwy::HWY_NAMESPACE::Lanes(d);
    auto v = hwy::HWY_NAMESPACE::Set(d, value);

    uint32_t i = 0u;
    for (; i + (uint32_t)lanes <= count; i += (uint32_t)lanes) {
        hwy::HWY_NAMESPACE::StoreU(v, d, values + i);
    }
    for (; i < count; ++i) {
        values[i] = value;
    }
}
