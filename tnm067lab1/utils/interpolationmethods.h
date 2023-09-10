#pragma once

#include <modules/tnm067lab1/tnm067lab1moduledefine.h>
#include <inviwo/core/util/glm.h>


namespace inviwo {

template <typename T>
struct float_type {
    using type = double;
};

template <>
struct float_type<float> {
    using type = float;
};
template <>
struct float_type<vec3> {
    using type = float;
};
template <>
struct float_type<vec2> {
    using type = float;
};
template <>
struct float_type<vec4> {
    using type = float;
};

namespace TNM067 {
namespace Interpolation {

#define ENABLE_LINEAR_UNITTEST 0
template <typename T, typename F = double>
T linear(const T& a, const T& b, F x) {
    if (x <= 0) return a;
    if (x >= 1) return b;

    return a * (1.0 - x) + b * x;
}

// clang-format off
    /*
     2------3
     |      |
    y|  •   |
     |      |
     0------1
        x
    */
    // clang format on
#define ENABLE_BILINEAR_UNITTEST 0
template<typename T, typename F = double> 
T bilinear(const std::array<T, 4> &v, F x, F y) {
    T top_row = linear(v[0], v[1], x);
    T bottom_row = linear(v[2], v[3], x);
    
    return linear(top_row, bottom_row, y);
}


    // clang-format off
    /* 
    a--•----b------c
    0  x    1      2
    */
// clang-format on
#define ENABLE_QUADRATIC_UNITTEST 0
template <typename T, typename F = double>
T quadratic(const T& a, const T& b, const T& c, F x) {

     return (1.0 - x) * (1.0 - 2 * x) * a + 4 * x * (1.0 - x) * b + x * (2.0 * x - 1.0) * c;
}

// clang-format off
    /* 
    6-------7-------8
    |       |       |
    |       |       |
    |       |       |
    3-------4-------5
    |       |       |
   y|  •    |       |
    |       |       |
    0-------1-------2
    0  x    1       2
    */
// clang-format on
#define ENABLE_BIQUADRATIC_UNITTEST 0
template <typename T, typename F = double>
T biQuadratic(const std::array<T, 9>& v, F x, F y) {

    T first_row = quadratic(v[0], v[1], v[2], x);
    T second_row = quadratic(v[3], v[4], v[5], x);
    T third_row = quadratic(v[6], v[7], v[8], x);

    return quadratic(first_row, second_row, third_row, y);
}

// clang-format off
    /*
     2---------3
     |'-.      |
     |   -,    |
   y |  •  -,  |
     |       -,|
     0---------1
        x
    */
// clang-format on
#define ENABLE_BARYCENTRIC_UNITTEST 1
template <typename T, typename F = double>
T barycentric(const std::array<T, 4>& v, F x, F y) {
    float alpha, beta, gamma;

    if (x + y < 1.f)  // 012 triangle
    {
        alpha = 1.0f - (x + y);
        beta = x;
        gamma = y;

        T t2 = v[0] * alpha + v[1] * beta + v[2] * gamma;

        return t2;

    } else {   //123 triangle
        alpha = (x + y) - 1.0f;
        beta = 1 - y;
        gamma = 1 - x;

        T t1 = v[3] * alpha + v[1] * beta + v[2] * gamma;

        return t1;
    }
}

}  // namespace Interpolation
}  // namespace TNM067
}  // namespace inviwo
