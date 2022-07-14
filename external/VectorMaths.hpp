#ifndef VectorMaths_hpp
#define VectorMaths_hpp

#include <iostream>
#include <cmath>
#include <array>
#include <sycl/sycl.hpp>

using vecType = std::array<float, 2>;

SYCL_EXTERNAL vecType operator*=(vecType& a, vecType b);
SYCL_EXTERNAL vecType operator*(vecType a, vecType b);

SYCL_EXTERNAL vecType operator+=(vecType& a, vecType b);
SYCL_EXTERNAL vecType operator+(vecType a, vecType b);

SYCL_EXTERNAL vecType operator-=(vecType& a, vecType b);
SYCL_EXTERNAL vecType operator-(vecType a, vecType b);

SYCL_EXTERNAL vecType operator*=(vecType& a, float s);
SYCL_EXTERNAL vecType operator*(vecType a, float s);
SYCL_EXTERNAL vecType operator*=(float s, vecType& a);
SYCL_EXTERNAL vecType operator*(float s, vecType a);

SYCL_EXTERNAL vecType operator/=(vecType& a, float s);
SYCL_EXTERNAL vecType operator/(vecType a, float s);

#endif