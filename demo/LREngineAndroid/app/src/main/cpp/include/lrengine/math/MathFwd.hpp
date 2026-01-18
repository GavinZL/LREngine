#pragma once

#include <cstdint>

namespace lrengine {
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
using Vec2f = Vec2T<float>;
using Vec3f = Vec3T<float>;
using Vec4f = Vec4T<float>;

using Vec2i = Vec2T<int32_t>;
using Vec3i = Vec3T<int32_t>;
using Vec4i = Vec4T<int32_t>;

using Vec2u = Vec2T<uint32_t>;
using Vec3u = Vec3T<uint32_t>;
using Vec4u = Vec4T<uint32_t>;

using Mat3f = Mat3T<float>;
using Mat4f = Mat4T<float>;

using Quaternionf = QuaternionT<float>;
using Quatf = QuaternionT<float>;

// 双精度版本
using Vec2d = Vec2T<double>;
using Vec3d = Vec3T<double>;
using Vec4d = Vec4T<double>;

using Mat3d = Mat3T<double>;
using Mat4d = Mat4T<double>;

using Quaterniond = QuaternionT<double>;
using Quatd = QuaternionT<double>;

} // namespace math
} // namespace lrengine
