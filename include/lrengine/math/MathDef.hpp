#pragma once

#include <cmath>
#include <cstdint>
#include <algorithm>
#include <limits>

namespace hyengine {
namespace math {

/**
 * @brief 数学常量定义
 */
constexpr float PI = 3.14159265358979323846f;
constexpr float TWO_PI = 6.28318530717958647692f;
constexpr float HALF_PI = 1.57079632679489661923f;
constexpr float INV_PI = 0.31830988618379067154f;
constexpr float DEG_TO_RAD = 0.01745329251994329577f;  // PI / 180
constexpr float RAD_TO_DEG = 57.2957795130823208768f;  // 180 / PI
constexpr float EPSILON = 1e-6f;

/**
 * @brief 数学工具函数
 */
template<typename T>
inline T clamp(T value, T min, T max) {
    return std::max(min, std::min(max, value));
}

template<typename T>
inline T lerp(T a, T b, float t) {
    return a + (b - a) * t;
}

template<typename T>
inline T smoothstep(T edge0, T edge1, T x) {
    T t = clamp((x - edge0) / (edge1 - edge0), T(0), T(1));
    return t * t * (T(3) - T(2) * t);
}

inline float degToRad(float degrees) {
    return degrees * DEG_TO_RAD;
}

inline float radToDeg(float radians) {
    return radians * RAD_TO_DEG;
}

inline bool floatEqual(float a, float b, float epsilon = EPSILON) {
    return std::abs(a - b) < epsilon;
}

inline float sign(float value) {
    return (value > 0.0f) ? 1.0f : ((value < 0.0f) ? -1.0f : 0.0f);
}

inline float saturate(float value) {
    return clamp(value, 0.0f, 1.0f);
}

/**
 * @brief 向量维度枚举
 */
enum class VectorDim : uint8_t {
    Vec2 = 2,
    Vec3 = 3,
    Vec4 = 4
};

/**
 * @brief 矩阵维度枚举
 */
enum class MatrixDim : uint8_t {
    Mat2 = 2,
    Mat3 = 3,
    Mat4 = 4
};

/**
 * @brief 坐标系类型
 */
enum class CoordinateSystem : uint8_t {
    RightHanded,  // 右手坐标系 (OpenGL)
    LeftHanded    // 左手坐标系 (DirectX)
};

/**
 * @brief 旋转顺序
 */
enum class RotationOrder : uint8_t {
    XYZ,
    XZY,
    YXZ,
    YZX,
    ZXY,
    ZYX
};

/**
 * @brief 投影类型
 */
enum class ProjectionType : uint8_t {
    Perspective,   // 透视投影
    Orthographic   // 正交投影
};

/**
 * @brief 插值类型
 */
enum class InterpolationType : uint8_t {
    Linear,        // 线性插值
    Spherical,     // 球面线性插值 (用于四元数)
    Cubic,         // 立方插值
    Hermite        // Hermite插值
};

} // namespace math
} // namespace hyengine
