#ifndef HY_MATH_MAT3_HPP
#define HY_MATH_MAT3_HPP

#include "MathDef.hpp"
#include "Vec3.hpp"
#include <cmath>
#include <cassert>
#include <cstring>

namespace lrengine {
namespace math {

// 前置声明
template <typename T> class QuaternionT;

/**
 * @brief 3x3矩阵模板类
 * @tparam T 数值类型 (float, double等)
 * 
 * @note 使用行主序存储,符合OpenGL规范
 * 矩阵布局: m[row][col]
 */
template <typename T>
class Mat3T {
public:
    union {
        struct {
            T m00, m01, m02;
            T m10, m11, m12;
            T m20, m21, m22;
        };
        struct {
            Vec3T<T> row[3];
        };
        T m[9];
        T m_mat[3][3];
    };

public:
    // 构造函数
    Mat3T() {
        *this = identity();
    }

    explicit Mat3T(T arr[3][3]) {
        std::memcpy(m_mat[0], arr[0], 3 * sizeof(T));
        std::memcpy(m_mat[1], arr[1], 3 * sizeof(T));
        std::memcpy(m_mat[2], arr[2], 3 * sizeof(T));
    }

    explicit Mat3T(T (&float_array)[9]) {
        std::memcpy(&m[0], float_array, sizeof(T) * 9);
    }

    explicit Mat3T(T m00, T m01, T m02,
                   T m10, T m11, T m12,
                   T m20, T m21, T m22) {
        m_mat[0][0] = m00; m_mat[0][1] = m01; m_mat[0][2] = m02;
        m_mat[1][0] = m10; m_mat[1][1] = m11; m_mat[1][2] = m12;
        m_mat[2][0] = m20; m_mat[2][1] = m21; m_mat[2][2] = m22;
    }

    explicit Mat3T(const Vec3T<T>& row0, const Vec3T<T>& row1, const Vec3T<T>& row2) {
        row[0] = row0;
        row[1] = row1;
        row[2] = row2;
    }

    // 访问器
    T* operator[](size_t row_index) {
        assert(row_index < 3);
        return m_mat[row_index];
    }

    const T* operator[](size_t row_index) const {
        assert(row_index < 3);
        return m_mat[row_index];
    }

    Vec3T<T> getColumn(size_t col_index) const {
        assert(col_index < 3);
        return Vec3T<T>(m_mat[0][col_index], m_mat[1][col_index], m_mat[2][col_index]);
    }

    void setColumn(size_t col_index, const Vec3T<T>& vec) {
        assert(col_index < 3);
        m_mat[0][col_index] = vec.x;
        m_mat[1][col_index] = vec.y;
        m_mat[2][col_index] = vec.z;
    }

    void fromAxes(const Vec3T<T>& x_axis, const Vec3T<T>& y_axis, const Vec3T<T>& z_axis) {
        setColumn(0, x_axis);
        setColumn(1, y_axis);
        setColumn(2, z_axis);
    }

    // 比较运算符
    bool operator==(const Mat3T& rhs) const {
        for (size_t i = 0; i < 3; ++i) {
            for (size_t j = 0; j < 3; ++j) {
                if (m_mat[i][j] != rhs.m_mat[i][j])
                    return false;
            }
        }
        return true;
    }

    bool operator!=(const Mat3T& rhs) const {
        return !operator==(rhs);
    }

    // 算术运算符
    Mat3T operator+(const Mat3T& rhs) const {
        Mat3T sum;
        for (size_t i = 0; i < 3; ++i) {
            for (size_t j = 0; j < 3; ++j) {
                sum.m_mat[i][j] = m_mat[i][j] + rhs.m_mat[i][j];
            }
        }
        return sum;
    }

    Mat3T operator-(const Mat3T& rhs) const {
        Mat3T diff;
        for (size_t i = 0; i < 3; ++i) {
            for (size_t j = 0; j < 3; ++j) {
                diff.m_mat[i][j] = m_mat[i][j] - rhs.m_mat[i][j];
            }
        }
        return diff;
    }

    Mat3T operator*(const Mat3T& rhs) const {
        Mat3T prod;
        for (size_t i = 0; i < 3; ++i) {
            for (size_t j = 0; j < 3; ++j) {
                prod.m_mat[i][j] = m_mat[i][0] * rhs.m_mat[0][j] +
                                   m_mat[i][1] * rhs.m_mat[1][j] +
                                   m_mat[i][2] * rhs.m_mat[2][j];
            }
        }
        return prod;
    }

    // 矩阵 * 向量
    Vec3T<T> operator*(const Vec3T<T>& rhs) const {
        Vec3T<T> prod;
        for (size_t i = 0; i < 3; ++i) {
            prod.v[i] = m_mat[i][0] * rhs.x + m_mat[i][1] * rhs.y + m_mat[i][2] * rhs.z;
        }
        return prod;
    }

    // 向量 * 矩阵
    friend Vec3T<T> operator*(const Vec3T<T>& lhs, const Mat3T& rhs) {
        Vec3T<T> prod;
        for (size_t i = 0; i < 3; ++i) {
            prod.v[i] = lhs.x * rhs.m_mat[0][i] + lhs.y * rhs.m_mat[1][i] + lhs.z * rhs.m_mat[2][i];
        }
        return prod;
    }

    Mat3T operator-() const {
        Mat3T neg;
        for (size_t i = 0; i < 3; ++i) {
            for (size_t j = 0; j < 3; ++j)
                neg.m_mat[i][j] = -m_mat[i][j];
        }
        return neg;
    }

    // 矩阵 * 标量
    Mat3T operator*(T scalar) const {
        Mat3T prod;
        for (size_t i = 0; i < 3; ++i) {
            for (size_t j = 0; j < 3; ++j)
                prod.m_mat[i][j] = scalar * m_mat[i][j];
        }
        return prod;
    }

    // 标量 * 矩阵
    friend Mat3T operator*(T scalar, const Mat3T& rhs) {
        Mat3T prod;
        for (size_t i = 0; i < 3; ++i) {
            for (size_t j = 0; j < 3; ++j)
                prod.m_mat[i][j] = scalar * rhs.m_mat[i][j];
        }
        return prod;
    }

    // 矩阵方法
    Mat3T transpose() const {
        Mat3T t;
        for (size_t i = 0; i < 3; ++i) {
            for (size_t j = 0; j < 3; ++j)
                t.m_mat[i][j] = m_mat[j][i];
        }
        return t;
    }

    T determinant() const {
        T cofactor00 = m_mat[1][1] * m_mat[2][2] - m_mat[1][2] * m_mat[2][1];
        T cofactor10 = m_mat[1][2] * m_mat[2][0] - m_mat[1][0] * m_mat[2][2];
        T cofactor20 = m_mat[1][0] * m_mat[2][1] - m_mat[1][1] * m_mat[2][0];

        return m_mat[0][0] * cofactor00 + m_mat[0][1] * cofactor10 + m_mat[0][2] * cofactor20;
    }

    bool inverse(Mat3T& inv_mat, T tolerance = static_cast<T>(1e-6)) const {
        T det = determinant();
        if (std::abs(det) <= tolerance)
            return false;

        inv_mat[0][0] = m_mat[1][1] * m_mat[2][2] - m_mat[1][2] * m_mat[2][1];
        inv_mat[0][1] = m_mat[0][2] * m_mat[2][1] - m_mat[0][1] * m_mat[2][2];
        inv_mat[0][2] = m_mat[0][1] * m_mat[1][2] - m_mat[0][2] * m_mat[1][1];
        inv_mat[1][0] = m_mat[1][2] * m_mat[2][0] - m_mat[1][0] * m_mat[2][2];
        inv_mat[1][1] = m_mat[0][0] * m_mat[2][2] - m_mat[0][2] * m_mat[2][0];
        inv_mat[1][2] = m_mat[0][2] * m_mat[1][0] - m_mat[0][0] * m_mat[1][2];
        inv_mat[2][0] = m_mat[1][0] * m_mat[2][1] - m_mat[1][1] * m_mat[2][0];
        inv_mat[2][1] = m_mat[0][1] * m_mat[2][0] - m_mat[0][0] * m_mat[2][1];
        inv_mat[2][2] = m_mat[0][0] * m_mat[1][1] - m_mat[0][1] * m_mat[1][0];

        T inv_det = static_cast<T>(1) / det;
        for (size_t i = 0; i < 3; ++i) {
            for (size_t j = 0; j < 3; ++j)
                inv_mat.m_mat[i][j] *= inv_det;
        }

        return true;
    }

    Mat3T inverse(T tolerance = static_cast<T>(1e-6)) const {
        Mat3T inv = zero();
        inverse(inv, tolerance);
        return inv;
    }

    // 静态方法
    static Mat3T identity() {
        return Mat3T(
            static_cast<T>(1), static_cast<T>(0), static_cast<T>(0),
            static_cast<T>(0), static_cast<T>(1), static_cast<T>(0),
            static_cast<T>(0), static_cast<T>(0), static_cast<T>(1)
        );
    }

    static Mat3T zero() {
        return Mat3T(
            static_cast<T>(0), static_cast<T>(0), static_cast<T>(0),
            static_cast<T>(0), static_cast<T>(0), static_cast<T>(0),
            static_cast<T>(0), static_cast<T>(0), static_cast<T>(0)
        );
    }

    static Mat3T scale(const Vec3T<T>& scale) {
        Mat3T mat = zero();
        mat.m_mat[0][0] = scale.x;
        mat.m_mat[1][1] = scale.y;
        mat.m_mat[2][2] = scale.z;
        return mat;
    }

    // 静态常量
    static const Mat3T ZERO;
    static const Mat3T IDENTITY;
};

// 静态常量定义
template <typename T>
const Mat3T<T> Mat3T<T>::ZERO = Mat3T<T>::zero();
template <typename T>
const Mat3T<T> Mat3T<T>::IDENTITY = Mat3T<T>::identity();

} // namespace math
} // namespace lrengine

#endif // HY_MATH_MAT3_HPP
