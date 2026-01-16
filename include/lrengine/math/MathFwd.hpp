#pragma once

#include <cstdint>

namespace hyengine {
namespace math {

// 前置声明向量类型
template<typename T> class Vec2T;
template<typename T> class Vec3T;
template<typename T> class Vec4T;

// 前置声明矩阵类型
template<typename T> class Mat3T;
template<typename T> class Mat4T;

// 前置声明四元数类型
template<typename T> class QuaternionT;

// 常用类型别名（使用float）
using Vec2 = Vec2T<float>;
using Vec3 = Vec3T<float>;
using Vec4 = Vec4T<float>;

using Vec2i = Vec2T<int32_t>;
using Vec3i = Vec3T<int32_t>;
using Vec4i = Vec4T<int32_t>;

using Vec2u = Vec2T<uint32_t>;
using Vec3u = Vec3T<uint32_t>;
using Vec4u = Vec4T<uint32_t>;

using Mat3 = Mat3T<float>;
using Mat4 = Mat4T<float>;

using Quaternion = QuaternionT<float>;
using Quat = QuaternionT<float>;

// 双精度版本
using Vec2d = Vec2T<double>;
using Vec3d = Vec3T<double>;
using Vec4d = Vec4T<double>;

using Mat3d = Mat3T<double>;
using Mat4d = Mat4T<double>;

using Quaterniond = QuaternionT<double>;

} // namespace math
} // namespace hyengine
