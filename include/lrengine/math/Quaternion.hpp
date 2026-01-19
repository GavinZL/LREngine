#ifndef HY_MATH_QUATERNION_HPP
#define HY_MATH_QUATERNION_HPP

#include "MathDef.hpp"
#include "Vec3.hpp"
#include "Mat3.hpp"
#include "Mat4.hpp"
#include <cmath>
#include <cassert>

namespace lrengine {
namespace math {

/**
 * @brief 四元数模板类
 * @tparam T 数值类型 (float, double等)
 * 
 * @note 四元数表示: q = w + xi + yj + zk
 * 用于表示3D旋转,避免万向节死锁问题
 * 
 * 与矩阵的转换基于列主序存储的旋转矩阵
 */
template <typename T>
class QuaternionT {
public:
    union {
        struct { T x, y, z, w; };
        struct { Vec3T<T> v; T s; };
        T q[4];
    };

public:
    // 构造函数
    QuaternionT() : x(0), y(0), z(0), w(1) {}
    QuaternionT(T x_, T y_, T z_, T w_) : x(x_), y(y_), z(z_), w(w_) {}

    explicit QuaternionT(const Mat3T<T>& rot) {
        fromRotationMatrix(rot);
    }

    QuaternionT(T radian, const Vec3T<T>& axis) {
        fromAngleAxis(radian, axis);
    }

    QuaternionT(const Vec3T<T>& xaxis, const Vec3T<T>& yaxis, const Vec3T<T>& zaxis) {
        fromAxes(xaxis, yaxis, zaxis);
    }

    // 从旋转矩阵构造 (列主序: rotation[col][row])
    void fromRotationMatrix(const Mat3T<T>& rotation) {
        // 列主序下读取对角线元素
        T trace = rotation[0][0] + rotation[1][1] + rotation[2][2];
        T root;

        if (trace > static_cast<T>(0)) {
            root = std::sqrt(trace + static_cast<T>(1));
            w = static_cast<T>(0.5) * root;
            root = static_cast<T>(0.5) / root;
            // 列主序: rotation[col][row]
            x = (rotation[1][2] - rotation[2][1]) * root;
            y = (rotation[2][0] - rotation[0][2]) * root;
            z = (rotation[0][1] - rotation[1][0]) * root;
        } else {
            size_t s_iNext[3] = {1, 2, 0};
            size_t i = 0;
            if (rotation[1][1] > rotation[0][0]) i = 1;
            if (rotation[2][2] > rotation[i][i]) i = 2;
            size_t j = s_iNext[i];
            size_t k = s_iNext[j];

            root = std::sqrt(rotation[i][i] - rotation[j][j] - rotation[k][k] + static_cast<T>(1));
            T* apkQuat[3] = {&x, &y, &z};
            *apkQuat[i] = static_cast<T>(0.5) * root;
            root = static_cast<T>(0.5) / root;
            // 列主序: rotation[col][row]
            w = (rotation[j][k] - rotation[k][j]) * root;
            *apkQuat[j] = (rotation[i][j] + rotation[j][i]) * root;
            *apkQuat[k] = (rotation[i][k] + rotation[k][i]) * root;
        }
    }

    // 转换为旋转矩阵 (列主序: kRot[col][row])
    void toRotationMatrix(Mat3T<T>& kRot) const {
        T _2x = x + x;
        T _2y = y + y;
        T _2z = z + z;
        T _2w = w + w;

        // 列主序: kRot[col][row]
        // 第0列
        kRot[0][0] = static_cast<T>(1) - _2y * y - _2z * z;
        kRot[0][1] = _2x * y + _2w * z;
        kRot[0][2] = _2x * z - _2w * y;
        // 第1列
        kRot[1][0] = _2x * y - _2w * z;
        kRot[1][1] = static_cast<T>(1) - _2x * x - _2z * z;
        kRot[1][2] = _2y * z + _2w * x;
        // 第2列
        kRot[2][0] = _2x * z + _2w * y;
        kRot[2][1] = _2y * z - _2w * x;
        kRot[2][2] = static_cast<T>(1) - _2x * x - _2y * y;
    }

    void toRotationMatrix(Mat4T<T>& kRot) const {
        T _2x = x + x;
        T _2y = y + y;
        T _2z = z + z;
        T _2w = w + w;

        // 列主序: kRot[col][row]
        // 第0列
        kRot[0][0] = static_cast<T>(1) - _2y * y - _2z * z;
        kRot[0][1] = _2x * y + _2w * z;
        kRot[0][2] = _2x * z - _2w * y;
        kRot[0][3] = static_cast<T>(0);
        // 第1列
        kRot[1][0] = _2x * y - _2w * z;
        kRot[1][1] = static_cast<T>(1) - _2x * x - _2z * z;
        kRot[1][2] = _2y * z + _2w * x;
        kRot[1][3] = static_cast<T>(0);
        // 第2列
        kRot[2][0] = _2x * z + _2w * y;
        kRot[2][1] = _2y * z - _2w * x;
        kRot[2][2] = static_cast<T>(1) - _2x * x - _2y * y;
        kRot[2][3] = static_cast<T>(0);
        // 第3列
        kRot[3][0] = static_cast<T>(0);
        kRot[3][1] = static_cast<T>(0);
        kRot[3][2] = static_cast<T>(0);
        kRot[3][3] = static_cast<T>(1);
    }

    // 从角度轴构造
    void fromAngleAxis(T radian, const Vec3T<T>& axis) {
        T half_angle = static_cast<T>(0.5) * radian;
        T sin_v = std::sin(half_angle);
        x = sin_v * axis.x;
        y = sin_v * axis.y;
        z = sin_v * axis.z;
        w = std::cos(half_angle);
    }

    void toAngleAxis(T& radian, Vec3T<T>& axis) const {
        T sqr_len = x * x + y * y + z * z;
        if (sqr_len > static_cast<T>(0)) {
            radian = static_cast<T>(2) * std::acos(w);
            T inv_len = static_cast<T>(1) / std::sqrt(sqr_len);
            axis.x = x * inv_len;
            axis.y = y * inv_len;
            axis.z = z * inv_len;
        } else {
            radian = static_cast<T>(0);
            axis.x = static_cast<T>(1);
            axis.y = static_cast<T>(0);
            axis.z = static_cast<T>(0);
        }
    }

    // 从轴构造 (列主序: rot[col][row])
    void fromAxes(const Vec3T<T>& xaxis, const Vec3T<T>& yaxis, const Vec3T<T>& zaxis) {
        Mat3T<T> rot;
        // 列主序: 每个轴向量存储为一列
        rot[0][0] = xaxis.x; rot[0][1] = xaxis.y; rot[0][2] = xaxis.z;
        rot[1][0] = yaxis.x; rot[1][1] = yaxis.y; rot[1][2] = yaxis.z;
        rot[2][0] = zaxis.x; rot[2][1] = zaxis.y; rot[2][2] = zaxis.z;
        fromRotationMatrix(rot);
    }

    void toAxes(Vec3T<T>& xaxis, Vec3T<T>& yaxis, Vec3T<T>& zaxis) const {
        Mat3T<T> rot;
        toRotationMatrix(rot);
        // 列主序: 每列为一个轴向量
        xaxis.x = rot[0][0]; xaxis.y = rot[0][1]; xaxis.z = rot[0][2];
        yaxis.x = rot[1][0]; yaxis.y = rot[1][1]; yaxis.z = rot[1][2];
        zaxis.x = rot[2][0]; zaxis.y = rot[2][1]; zaxis.z = rot[2][2];
    }

    // 获取各轴
    Vec3T<T> xAxis() const {
        T ty = static_cast<T>(2) * y;
        T tz = static_cast<T>(2) * z;
        T twy = ty * w;
        T twz = tz * w;
        T txy = ty * x;
        T txz = tz * x;
        T tyy = ty * y;
        T tzz = tz * z;
        return Vec3T<T>(static_cast<T>(1) - (tyy + tzz), txy + twz, txz - twy);
    }

    Vec3T<T> yAxis() const {
        T tx = static_cast<T>(2) * x;
        T ty = static_cast<T>(2) * y;
        T tz = static_cast<T>(2) * z;
        T twx = tx * w;
        T twz = tz * w;
        T txx = tx * x;
        T txy = ty * x;
        T tyz = tz * y;
        T tzz = tz * z;
        return Vec3T<T>(txy - twz, static_cast<T>(1) - (txx + tzz), tyz + twx);
    }

    Vec3T<T> zAxis() const {
        T tx = static_cast<T>(2) * x;
        T ty = static_cast<T>(2) * y;
        T tz = static_cast<T>(2) * z;
        T twx = tx * w;
        T twy = ty * w;
        T txx = tx * x;
        T txz = tz * x;
        T tyy = ty * y;
        T tyz = tz * y;
        return Vec3T<T>(txz + twy, tyz - twx, static_cast<T>(1) - (txx + tyy));
    }

    // 运算符
    QuaternionT operator+(const QuaternionT& rhs) const {
        return QuaternionT(x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w);
    }

    QuaternionT operator-(const QuaternionT& rhs) const {
        return QuaternionT(x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w);
    }

    QuaternionT operator*(const QuaternionT& rhs) const {
        return QuaternionT(
            w * rhs.x + x * rhs.w + y * rhs.z - z * rhs.y,
            w * rhs.y + y * rhs.w + z * rhs.x - x * rhs.z,
            w * rhs.z + z * rhs.w + x * rhs.y - y * rhs.x,
            w * rhs.w - x * rhs.x - y * rhs.y - z * rhs.z
        );
    }

    QuaternionT operator*(T scalar) const {
        return QuaternionT(x * scalar, y * scalar, z * scalar, w * scalar);
    }

    QuaternionT operator/(T scalar) const {
        assert(scalar != static_cast<T>(0));
        return QuaternionT(x / scalar, y / scalar, z / scalar, w / scalar);
    }

    friend QuaternionT operator*(T scalar, const QuaternionT& rhs) {
        return QuaternionT(scalar * rhs.x, scalar * rhs.y, scalar * rhs.z, scalar * rhs.w);
    }

    QuaternionT operator-() const {
        return QuaternionT(-x, -y, -z, -w);
    }

    bool operator==(const QuaternionT& rhs) const {
        return (rhs.x == x) && (rhs.y == y) && (rhs.z == z) && (rhs.w == w);
    }

    bool operator!=(const QuaternionT& rhs) const {
        return !operator==(rhs);
    }

    // 向量旋转
    Vec3T<T> operator*(const Vec3T<T>& v) const {
        Vec3T<T> qvec(x, y, z);
        Vec3T<T> uv = qvec.crossProduct(v);
        Vec3T<T> uuv = qvec.crossProduct(uv);
        uv = uv * (static_cast<T>(2) * w);
        uuv = uuv * static_cast<T>(2);
        return v + uv + uuv;
    }

    // 四元数方法
    T dot(const QuaternionT& rkQ) const {
        return w * rkQ.w + x * rkQ.x + y * rkQ.y + z * rkQ.z;
    }

    T length() const {
        return std::sqrt(w * w + x * x + y * y + z * z);
    }

    void normalise() {
        T factor = static_cast<T>(1) / length();
        x *= factor;
        y *= factor;
        z *= factor;
        w *= factor;
    }

    QuaternionT inverse() const {
        T norm = w * w + x * x + y * y + z * z;
        if (norm > static_cast<T>(0)) {
            T inv_norm = static_cast<T>(1) / norm;
            return QuaternionT(-x * inv_norm, -y * inv_norm, -z * inv_norm, w * inv_norm);
        } else {
            return ZERO;
        }
    }

    QuaternionT conjugate() const {
        return QuaternionT(-x, -y, -z, w);
    }

    bool isIdentity() const {
        return std::abs(x) < EPSILON && std::abs(y) < EPSILON && 
               std::abs(z) < EPSILON && std::abs(w - static_cast<T>(1)) < EPSILON;
    }

    // 球面线性插值
    static QuaternionT sLerp(T t, const QuaternionT& kp, const QuaternionT& kq, bool shortest_path = false) {
        T cos_v = kp.dot(kq);
        QuaternionT kt;

        if (cos_v < static_cast<T>(0) && shortest_path) {
            cos_v = -cos_v;
            kt = -kq;
        } else {
            kt = kq;
        }

        if (std::abs(cos_v) < static_cast<T>(1) - static_cast<T>(1e-3)) {
            T sin_v = std::sqrt(static_cast<T>(1) - cos_v * cos_v);
            T angle = std::atan2(sin_v, cos_v);
            T inv_sin = static_cast<T>(1) / sin_v;
            T coeff0 = std::sin((static_cast<T>(1) - t) * angle) * inv_sin;
            T coeff1 = std::sin(t * angle) * inv_sin;
            return coeff0 * kp + coeff1 * kt;
        } else {
            QuaternionT r = (static_cast<T>(1) - t) * kp + t * kt;
            r.normalise();
            return r;
        }
    }

    // 归一化线性插值
    static QuaternionT nLerp(T t, const QuaternionT& kp, const QuaternionT& kq, bool shortest_path = false) {
        QuaternionT result;
        T cos_value = kp.dot(kq);
        if (cos_value < static_cast<T>(0) && shortest_path) {
            result = kp + t * ((-kq) - kp);
        } else {
            result = kp + t * (kq - kp);
        }
        result.normalise();
        return result;
    }

    // 静态工厂方法
    static QuaternionT identity() {
        return QuaternionT(static_cast<T>(0), static_cast<T>(0), static_cast<T>(0), static_cast<T>(1));
    }

    // 静态常量
    static const QuaternionT ZERO;
    static const QuaternionT IDENTITY;
};

// 静态常量定义
template <typename T>
const QuaternionT<T> QuaternionT<T>::ZERO = QuaternionT<T>(0, 0, 0, 0);
template <typename T>
const QuaternionT<T> QuaternionT<T>::IDENTITY = QuaternionT<T>(0, 0, 0, 1);

} // namespace math
} // namespace lrengine

#endif // HY_MATH_QUATERNION_HPP
