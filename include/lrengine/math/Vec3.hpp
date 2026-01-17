#ifndef HY_MATH_VEC3_HPP
#define HY_MATH_VEC3_HPP

#include "MathDef.hpp"
#include <cmath>
#include <cassert>

namespace lrengine {
namespace math {

/**
 * @brief 3D向量模板类
 * @tparam T 数值类型 (float, double, int32_t等)
 */
template <typename T>
class Vec3T {
public:
    union {
        struct { T x, y, z; };
        struct { T r, g, b; };  // 颜色访问
        T v[3];
    };

public:
    // 构造函数
    Vec3T() = default;
    Vec3T(T x_, T y_, T z_) : x(x_), y(y_), z(z_) {}
    explicit Vec3T(T scaler) : x(scaler), y(scaler), z(scaler) {}
    explicit Vec3T(const T v[3]) : x(v[0]), y(v[1]), z(v[2]) {}
    explicit Vec3T(T* const r) : x(r[0]), y(r[1]), z(r[2]) {}

    // 比较运算符
    bool operator==(const Vec3T& rhs) const {
        return (x == rhs.x && y == rhs.y && z == rhs.z);
    }
    bool operator!=(const Vec3T& rhs) const {
        return (x != rhs.x || y != rhs.y || z != rhs.z);
    }

    // 算术运算符
    Vec3T operator+(const Vec3T& rhs) const {
        return Vec3T(x + rhs.x, y + rhs.y, z + rhs.z);
    }
    Vec3T operator-(const Vec3T& rhs) const {
        return Vec3T(x - rhs.x, y - rhs.y, z - rhs.z);
    }
    Vec3T operator*(T scalar) const {
        return Vec3T(x * scalar, y * scalar, z * scalar);
    }
    Vec3T operator*(const Vec3T& rhs) const {
        return Vec3T(x * rhs.x, y * rhs.y, z * rhs.z);
    }
    Vec3T operator/(T scale) const {
        assert(scale != 0);
        T inv = static_cast<T>(1) / scale;
        return Vec3T(x * inv, y * inv, z * inv);
    }
    Vec3T operator/(const Vec3T& rhs) const {
        return Vec3T(x / rhs.x, y / rhs.y, z / rhs.z);
    }

    const Vec3T& operator+() const { return *this; }
    Vec3T operator-() const { return Vec3T(-x, -y, -z); }

    // 友元运算符
    friend Vec3T operator*(T scalar, const Vec3T& rhs) {
        return Vec3T(scalar * rhs.x, scalar * rhs.y, scalar * rhs.z);
    }
    friend Vec3T operator/(T fScalar, const Vec3T& rhs) {
        return Vec3T(fScalar / rhs.x, fScalar / rhs.y, fScalar / rhs.z);
    }
    friend Vec3T operator+(const Vec3T& lhs, T rhs) {
        return Vec3T(lhs.x + rhs, lhs.y + rhs, lhs.z + rhs);
    }
    friend Vec3T operator+(T lhs, const Vec3T& rhs) {
        return Vec3T(lhs + rhs.x, lhs + rhs.y, lhs + rhs.z);
    }
    friend Vec3T operator-(const Vec3T& lhs, T rhs) {
        return Vec3T(lhs.x - rhs, lhs.y - rhs, lhs.z - rhs);
    }
    friend Vec3T operator-(T lhs, const Vec3T& rhs) {
        return Vec3T(lhs - rhs.x, lhs - rhs.y, lhs - rhs.z);
    }

    // 复合赋值运算符
    Vec3T& operator+=(const Vec3T& rhs) {
        x += rhs.x; y += rhs.y; z += rhs.z;
        return *this;
    }
    Vec3T& operator+=(T scalar) {
        x += scalar; y += scalar; z += scalar;
        return *this;
    }
    Vec3T& operator-=(const Vec3T& rhs) {
        x -= rhs.x; y -= rhs.y; z -= rhs.z;
        return *this;
    }
    Vec3T& operator-=(T scalar) {
        x -= scalar; y -= scalar; z -= scalar;
        return *this;
    }
    Vec3T& operator*=(T scalar) {
        x *= scalar; y *= scalar; z *= scalar;
        return *this;
    }
    Vec3T& operator*=(const Vec3T& rhs) {
        x *= rhs.x; y *= rhs.y; z *= rhs.z;
        return *this;
    }
    Vec3T& operator/=(T scalar) {
        assert(scalar != 0);
        T inv = static_cast<T>(1) / scalar;
        x *= inv; y *= inv; z *= inv;
        return *this;
    }
    Vec3T& operator/=(const Vec3T& rhs) {
        x /= rhs.x; y /= rhs.y; z /= rhs.z;
        return *this;
    }

    // 向量方法
    Vec3T midPoint(const Vec3T& vec) const {
        return Vec3T((x + vec.x) * static_cast<T>(0.5),
                     (y + vec.y) * static_cast<T>(0.5),
                     (z + vec.z) * static_cast<T>(0.5));
    }

    bool operator<(const Vec3T& rhs) const {
        return x < rhs.x && y < rhs.y && z < rhs.z;
    }
    bool operator>(const Vec3T& rhs) const {
        return x > rhs.x && y > rhs.y && z > rhs.z;
    }

    T length() const {
        return std::sqrt(squaredLength());
    }
    T squaredLength() const {
        return x * x + y * y + z * z;
    }

    T distance(const Vec3T& rhs) const {
        return (*this - rhs).length();
    }
    T squaredDistance(const Vec3T& rhs) const {
        return (*this - rhs).squaredLength();
    }

    T dotProduct(const Vec3T& vec) const {
        return x * vec.x + y * vec.y + z * vec.z;
    }
    
    Vec3T crossProduct(const Vec3T& rhs) const {
        return Vec3T(
            y * rhs.z - z * rhs.y,
            z * rhs.x - x * rhs.z,
            x * rhs.y - y * rhs.x
        );
    }

    Vec3T reflect(const Vec3T& normal) const {
        return Vec3T(*this - (static_cast<T>(2) * this->dotProduct(normal) * normal));
    }

    T normalise() {
        T len = length();
        if (len > static_cast<T>(0)) {
            T inv_length = static_cast<T>(1) / len;
            x *= inv_length;
            y *= inv_length;
            z *= inv_length;
        }
        return len;
    }

    Vec3T normalisedCopy() const {
        Vec3T ret = *this;
        ret.normalise();
        return ret;
    }

    // 访问器
    T getX() const { return x; }
    T getY() const { return y; }
    T getZ() const { return z; }
    void setX(T value) { x = value; }
    void setY(T value) { y = value; }
    void setZ(T value) { z = value; }

    bool isZeroLength() const {
        T sqlen = (x * x) + (y * y) + (z * z);
        return (sqlen < (EPSILON * EPSILON));
    }

    // 静态方法
    static Vec3T lerp(const Vec3T& lhs, const Vec3T& rhs, T alpha) {
        return lhs + alpha * (rhs - lhs);
    }

    // 静态常量
    static const Vec3T ZERO;
    static const Vec3T UNIT_X;
    static const Vec3T UNIT_Y;
    static const Vec3T UNIT_Z;
    static const Vec3T NEGATIVE_UNIT_X;
    static const Vec3T NEGATIVE_UNIT_Y;
    static const Vec3T NEGATIVE_UNIT_Z;
    static const Vec3T UNIT_SCALE;
};

// 静态常量定义
template <typename T>
const Vec3T<T> Vec3T<T>::ZERO = Vec3T<T>(0, 0, 0);
template <typename T>
const Vec3T<T> Vec3T<T>::UNIT_X = Vec3T<T>(1, 0, 0);
template <typename T>
const Vec3T<T> Vec3T<T>::UNIT_Y = Vec3T<T>(0, 1, 0);
template <typename T>
const Vec3T<T> Vec3T<T>::UNIT_Z = Vec3T<T>(0, 0, 1);
template <typename T>
const Vec3T<T> Vec3T<T>::NEGATIVE_UNIT_X = Vec3T<T>(-1, 0, 0);
template <typename T>
const Vec3T<T> Vec3T<T>::NEGATIVE_UNIT_Y = Vec3T<T>(0, -1, 0);
template <typename T>
const Vec3T<T> Vec3T<T>::NEGATIVE_UNIT_Z = Vec3T<T>(0, 0, -1);
template <typename T>
const Vec3T<T> Vec3T<T>::UNIT_SCALE = Vec3T<T>(1, 1, 1);

} // namespace math
} // namespace lrengine

#endif // HY_MATH_VEC3_HPP
