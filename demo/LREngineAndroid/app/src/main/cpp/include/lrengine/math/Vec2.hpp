#ifndef HY_MATH_VEC2_HPP
#define HY_MATH_VEC2_HPP

#include "MathDef.hpp"
#include <cmath>
#include <cassert>

namespace lrengine {
namespace math {

/**
 * @brief 2D向量模板类
 * @tparam T 数值类型 (float, double, int32_t等)
 */
template <typename T>
class Vec2T {
public:
    union {
        struct { T x, y; };
        struct { T s, t; };         // 纹理坐标访问
        struct { T width, height; }; // 尺寸访问
        T v[2];
    };

public:
    // 构造函数
    Vec2T() = default;
    Vec2T(T x_, T y_) : x(x_), y(y_) {}
    explicit Vec2T(T scaler) : x(scaler), y(scaler) {}
    explicit Vec2T(const T v[2]) : x(v[0]), y(v[1]) {}
    explicit Vec2T(T* const r) : x(r[0]), y(r[1]) {}

    // 比较运算符
    bool operator==(const Vec2T& rhs) const {
        return (x == rhs.x && y == rhs.y);
    }
    bool operator!=(const Vec2T& rhs) const {
        return (x != rhs.x || y != rhs.y);
    }

    // 算术运算符
    Vec2T operator+(const Vec2T& rhs) const {
        return Vec2T(x + rhs.x, y + rhs.y);
    }
    Vec2T operator-(const Vec2T& rhs) const {
        return Vec2T(x - rhs.x, y - rhs.y);
    }
    Vec2T operator*(T scalar) const {
        return Vec2T(x * scalar, y * scalar);
    }
    Vec2T operator*(const Vec2T& rhs) const {
        return Vec2T(x * rhs.x, y * rhs.y);
    }
    Vec2T operator/(T scale) const {
        assert(scale != 0);
        T inv = static_cast<T>(1) / scale;
        return Vec2T(x * inv, y * inv);
    }
    Vec2T operator/(const Vec2T& rhs) const {
        return Vec2T(x / rhs.x, y / rhs.y);
    }

    const Vec2T& operator+() const { return *this; }
    Vec2T operator-() const { return Vec2T(-x, -y); }

    // 友元运算符
    friend Vec2T operator*(T scalar, const Vec2T& rhs) {
        return Vec2T(scalar * rhs.x, scalar * rhs.y);
    }
    friend Vec2T operator/(T fScalar, const Vec2T& rhs) {
        return Vec2T(fScalar / rhs.x, fScalar / rhs.y);
    }
    friend Vec2T operator+(const Vec2T& lhs, T rhs) {
        return Vec2T(lhs.x + rhs, lhs.y + rhs);
    }
    friend Vec2T operator+(T lhs, const Vec2T& rhs) {
        return Vec2T(lhs + rhs.x, lhs + rhs.y);
    }
    friend Vec2T operator-(const Vec2T& lhs, T rhs) {
        return Vec2T(lhs.x - rhs, lhs.y - rhs);
    }
    friend Vec2T operator-(T lhs, const Vec2T& rhs) {
        return Vec2T(lhs - rhs.x, lhs - rhs.y);
    }

    // 复合赋值运算符
    Vec2T& operator+=(const Vec2T& rhs) {
        x += rhs.x; y += rhs.y;
        return *this;
    }
    Vec2T& operator+=(T scalar) {
        x += scalar; y += scalar;
        return *this;
    }
    Vec2T& operator-=(const Vec2T& rhs) {
        x -= rhs.x; y -= rhs.y;
        return *this;
    }
    Vec2T& operator-=(T scalar) {
        x -= scalar; y -= scalar;
        return *this;
    }
    Vec2T& operator*=(T scalar) {
        x *= scalar; y *= scalar;
        return *this;
    }
    Vec2T& operator*=(const Vec2T& rhs) {
        x *= rhs.x; y *= rhs.y;
        return *this;
    }
    Vec2T& operator/=(T scalar) {
        assert(scalar != 0);
        T inv = static_cast<T>(1) / scalar;
        x *= inv; y *= inv;
        return *this;
    }
    Vec2T& operator/=(const Vec2T& rhs) {
        x /= rhs.x; y /= rhs.y;
        return *this;
    }

    // 向量方法
    Vec2T midPoint(const Vec2T& vec) const {
        return Vec2T((x + vec.x) * static_cast<T>(0.5), 
                     (y + vec.y) * static_cast<T>(0.5));
    }

    bool operator<(const Vec2T& rhs) const {
        return x < rhs.x && y < rhs.y;
    }
    bool operator>(const Vec2T& rhs) const {
        return x > rhs.x && y > rhs.y;
    }

    T length() const {
        return std::sqrt(squaredLength());
    }
    T squaredLength() const {
        return x * x + y * y;
    }

    T distance(const Vec2T& rhs) const {
        return (*this - rhs).length();
    }
    T squaredDistance(const Vec2T& rhs) const {
        return (*this - rhs).squaredLength();
    }

    T dotProduct(const Vec2T& vec) const {
        return x * vec.x + y * vec.y;
    }
    T crossProduct(const Vec2T& rhs) const {
        return x * rhs.y - y * rhs.x;
    }

    Vec2T reflect(const Vec2T& normal) const {
        return Vec2T(*this - (static_cast<T>(2) * this->dotProduct(normal) * normal));
    }

    T normalise() {
        T len = length();
        if (len > static_cast<T>(0)) {
            T inv_length = static_cast<T>(1) / len;
            x *= inv_length;
            y *= inv_length;
        }
        return len;
    }

    Vec2T normalisedCopy() const {
        Vec2T ret = *this;
        ret.normalise();
        return ret;
    }

    // 访问器
    T getX() const { return x; }
    T getY() const { return y; }
    void setX(T value) { x = value; }
    void setY(T value) { y = value; }

    bool isZeroLength() const {
        T sqlen = (x * x) + (y * y);
        return (sqlen < (EPSILON * EPSILON));
    }

    // 静态方法
    static Vec2T lerp(const Vec2T& lhs, const Vec2T& rhs, T alpha) {
        return lhs + alpha * (rhs - lhs);
    }

    // 静态常量
    static const Vec2T ZERO;
    static const Vec2T UNIT_X;
    static const Vec2T UNIT_Y;
    static const Vec2T NEGATIVE_UNIT_X;
    static const Vec2T NEGATIVE_UNIT_Y;
    static const Vec2T UNIT_SCALE;
};

// 静态常量定义
template <typename T>
const Vec2T<T> Vec2T<T>::ZERO = Vec2T<T>(0, 0);
template <typename T>
const Vec2T<T> Vec2T<T>::UNIT_X = Vec2T<T>(1, 0);
template <typename T>
const Vec2T<T> Vec2T<T>::UNIT_Y = Vec2T<T>(0, 1);
template <typename T>
const Vec2T<T> Vec2T<T>::NEGATIVE_UNIT_X = Vec2T<T>(-1, 0);
template <typename T>
const Vec2T<T> Vec2T<T>::NEGATIVE_UNIT_Y = Vec2T<T>(0, -1);
template <typename T>
const Vec2T<T> Vec2T<T>::UNIT_SCALE = Vec2T<T>(1, 1);

} // namespace math
} // namespace lrengine

#endif // HY_MATH_VEC2_HPP
