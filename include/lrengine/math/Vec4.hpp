#ifndef HY_MATH_VEC4_HPP
#define HY_MATH_VEC4_HPP

#include "MathDef.hpp"
#include "Vec3.hpp"
#include "Vec2.hpp"
#include <cmath>
#include <cassert>

namespace lrengine {
namespace math {

/**
 * @brief 4D向量模板类
 * @tparam T 数值类型 (float, double, int32_t等)
 */
template <typename T>
class Vec4T {
public:
    union {
        struct { T x, y, z, w; };
        struct { T r, g, b, a; };  // 颜色访问
        struct { Vec2T<T> origin, size; };  // 矩形访问
        T v[4];
    };

public:
    // 构造函数
    Vec4T() = default;
    Vec4T(T x_, T y_, T z_, T w_) : x(x_), y(y_), z(z_), w(w_) {}
    Vec4T(const Vec3T<T>& v3, T w_) : x(v3.x), y(v3.y), z(v3.z), w(w_) {}
    explicit Vec4T(T scaler) : x(scaler), y(scaler), z(scaler), w(scaler) {}
    explicit Vec4T(T coords[4]) : x(coords[0]), y(coords[1]), z(coords[2]), w(coords[3]) {}

    Vec4T& operator=(T scalar) {
        x = scalar; y = scalar; z = scalar; w = scalar;
        return *this;
    }

    // 比较运算符
    bool operator==(const Vec4T& rhs) const {
        return (x == rhs.x && y == rhs.y && z == rhs.z && w == rhs.w);
    }
    bool operator!=(const Vec4T& rhs) const {
        return !(rhs == *this);
    }

    // 算术运算符
    Vec4T operator+(const Vec4T& rhs) const {
        return Vec4T(x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w);
    }
    Vec4T operator-(const Vec4T& rhs) const {
        return Vec4T(x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w);
    }
    Vec4T operator*(T scalar) const {
        return Vec4T(x * scalar, y * scalar, z * scalar, w * scalar);
    }
    Vec4T operator*(const Vec4T& rhs) const {
        return Vec4T(rhs.x * x, rhs.y * y, rhs.z * z, rhs.w * w);
    }
    Vec4T operator/(T scalar) const {
        assert(scalar != 0);
        return Vec4T(x / scalar, y / scalar, z / scalar, w / scalar);
    }
    Vec4T operator/(const Vec4T& rhs) const {
        assert(rhs.x != 0 && rhs.y != 0 && rhs.z != 0 && rhs.w != 0);
        return Vec4T(x / rhs.x, y / rhs.y, z / rhs.z, w / rhs.w);
    }

    const Vec4T& operator+() const { return *this; }
    Vec4T operator-() const { return Vec4T(-x, -y, -z, -w); }

    // 友元运算符
    friend Vec4T operator*(T scalar, const Vec4T& rhs) {
        return Vec4T(scalar * rhs.x, scalar * rhs.y, scalar * rhs.z, scalar * rhs.w);
    }
    friend Vec4T operator/(T scalar, const Vec4T& rhs) {
        assert(rhs.x != 0 && rhs.y != 0 && rhs.z != 0 && rhs.w != 0);
        return Vec4T(scalar / rhs.x, scalar / rhs.y, scalar / rhs.z, scalar / rhs.w);
    }
    friend Vec4T operator+(const Vec4T& lhs, T rhs) {
        return Vec4T(lhs.x + rhs, lhs.y + rhs, lhs.z + rhs, lhs.w + rhs);
    }
    friend Vec4T operator+(T lhs, const Vec4T& rhs) {
        return Vec4T(lhs + rhs.x, lhs + rhs.y, lhs + rhs.z, lhs + rhs.w);
    }
    friend Vec4T operator-(const Vec4T& lhs, T rhs) {
        return Vec4T(lhs.x - rhs, lhs.y - rhs, lhs.z - rhs, lhs.w - rhs);
    }
    friend Vec4T operator-(T lhs, const Vec4T& rhs) {
        return Vec4T(lhs - rhs.x, lhs - rhs.y, lhs - rhs.z, lhs - rhs.w);
    }

    // 复合赋值运算符
    Vec4T& operator+=(const Vec4T& rhs) {
        x += rhs.x; y += rhs.y; z += rhs.z; w += rhs.w;
        return *this;
    }
    Vec4T& operator-=(const Vec4T& rhs) {
        x -= rhs.x; y -= rhs.y; z -= rhs.z; w -= rhs.w;
        return *this;
    }
    Vec4T& operator*=(T scalar) {
        x *= scalar; y *= scalar; z *= scalar; w *= scalar;
        return *this;
    }
    Vec4T& operator+=(T scalar) {
        x += scalar; y += scalar; z += scalar; w += scalar;
        return *this;
    }
    Vec4T& operator-=(T scalar) {
        x -= scalar; y -= scalar; z -= scalar; w -= scalar;
        return *this;
    }
    Vec4T& operator*=(const Vec4T& rhs) {
        x *= rhs.x; y *= rhs.y; z *= rhs.z; w *= rhs.w;
        return *this;
    }
    Vec4T& operator/=(T scalar) {
        assert(scalar != 0);
        x /= scalar; y /= scalar; z /= scalar; w /= scalar;
        return *this;
    }
    Vec4T& operator/=(const Vec4T& rhs) {
        assert(rhs.x != 0 && rhs.y != 0 && rhs.z != 0 && rhs.w != 0);
        x /= rhs.x; y /= rhs.y; z /= rhs.z; w /= rhs.w;
        return *this;
    }

    // 向量方法
    T dotProduct(const Vec4T& vec) const {
        return x * vec.x + y * vec.y + z * vec.z + w * vec.w;
    }

    T length() const {
        return std::sqrt(squaredLength());
    }
    T squaredLength() const {
        return x * x + y * y + z * z + w * w;
    }

    T normalise() {
        T len = length();
        if (len > static_cast<T>(0)) {
            T inv_length = static_cast<T>(1) / len;
            x *= inv_length;
            y *= inv_length;
            z *= inv_length;
            w *= inv_length;
        }
        return len;
    }

    Vec4T normalisedCopy() const {
        Vec4T ret = *this;
        ret.normalise();
        return ret;
    }

    bool isZero() const {
        return x == static_cast<T>(0) && y == static_cast<T>(0) && 
               z == static_cast<T>(0) && w == static_cast<T>(0);
    }

    bool isZeroLength() const {
        T sqlen = (x * x) + (y * y) + (z * z) + (w * w);
        return (sqlen < (EPSILON * EPSILON));
    }

    // 静态方法
    static Vec4T lerp(const Vec4T& lhs, const Vec4T& rhs, T alpha) {
        return lhs + alpha * (rhs - lhs);
    }

    // 静态常量
    static const Vec4T ZERO;
    static const Vec4T UNIT_SCALE;
};

// 静态常量定义
template <typename T>
const Vec4T<T> Vec4T<T>::ZERO = Vec4T<T>(0, 0, 0, 0);
template <typename T>
const Vec4T<T> Vec4T<T>::UNIT_SCALE = Vec4T<T>(1, 1, 1, 1);

} // namespace math
} // namespace lrengine

#endif // HY_MATH_VEC4_HPP
