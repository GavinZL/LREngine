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
 * @note 使用列主序存储,符合OpenGL规范
 * 矩阵布局: m_mat[col][row], 内存连续按列排列
 * 
 * 矩阵内存布局:
 * | m00 m10 m20 |   内存顺序: m00, m01, m02, m10, m11, m12, m20, m21, m22
 * | m01 m11 m21 |   即: col0, col1, col2
 * | m02 m12 m22 |
 */
template <typename T>
class Mat3T {
public:
    union {
        struct {
            // 列主序: 按列存储
            T m00, m01, m02;  // 第0列
            T m10, m11, m12;  // 第1列
            T m20, m21, m22;  // 第2列
        };
        struct {
            Vec3T<T> col[3];  // 改为col表示列
        };
        T m[9];
        T m_mat[3][3];  // m_mat[col][row]
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

    // 从列向量构造
    explicit Mat3T(const Vec3T<T>& col0, const Vec3T<T>& col1, const Vec3T<T>& col2) {
        col[0] = col0;
        col[1] = col1;
        col[2] = col2;
    }

    // 访问器 - 返回列向量
    T* operator[](size_t col_index) {
        assert(col_index < 3);
        return m_mat[col_index];
    }

    const T* operator[](size_t col_index) const {
        assert(col_index < 3);
        return m_mat[col_index];
    }

    // 获取列向量 (列主序下直接返回连续内存)
    Vec3T<T> getColumn(size_t col_index) const {
        assert(col_index < 3);
        return col[col_index];
    }

    void setColumn(size_t col_index, const Vec3T<T>& vec) {
        assert(col_index < 3);
        col[col_index] = vec;
    }

    // 获取行向量
    Vec3T<T> getRow(size_t row_index) const {
        assert(row_index < 3);
        return Vec3T<T>(m_mat[0][row_index], m_mat[1][row_index], m_mat[2][row_index]);
    }

    void setRow(size_t row_index, const Vec3T<T>& vec) {
        assert(row_index < 3);
        m_mat[0][row_index] = vec.x;
        m_mat[1][row_index] = vec.y;
        m_mat[2][row_index] = vec.z;
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

    // 矩阵乘法 (列主序: C = A * B, C[col][row] = sum(A[k][row] * B[col][k]))
    Mat3T operator*(const Mat3T& rhs) const {
        Mat3T prod;
        for (size_t col = 0; col < 3; ++col) {
            for (size_t row = 0; row < 3; ++row) {
                prod.m_mat[col][row] = m_mat[0][row] * rhs.m_mat[col][0] +
                                       m_mat[1][row] * rhs.m_mat[col][1] +
                                       m_mat[2][row] * rhs.m_mat[col][2];
            }
        }
        return prod;
    }

    // 矩阵 * 向量 (v' = M * v)
    Vec3T<T> operator*(const Vec3T<T>& v) const {
        return Vec3T<T>(
            m_mat[0][0] * v.x + m_mat[1][0] * v.y + m_mat[2][0] * v.z,
            m_mat[0][1] * v.x + m_mat[1][1] * v.y + m_mat[2][1] * v.z,
            m_mat[0][2] * v.x + m_mat[1][2] * v.y + m_mat[2][2] * v.z
        );
    }

    // 向量 * 矩阵 (v' = v * M = M^T * v)
    friend Vec3T<T> operator*(const Vec3T<T>& v, const Mat3T& mat) {
        return Vec3T<T>(
            v.x * mat.m_mat[0][0] + v.y * mat.m_mat[0][1] + v.z * mat.m_mat[0][2],
            v.x * mat.m_mat[1][0] + v.y * mat.m_mat[1][1] + v.z * mat.m_mat[1][2],
            v.x * mat.m_mat[2][0] + v.y * mat.m_mat[2][1] + v.z * mat.m_mat[2][2]
        );
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
        for (size_t col = 0; col < 3; ++col) {
            for (size_t row = 0; row < 3; ++row)
                t.m_mat[col][row] = m_mat[row][col];
        }
        return t;
    }

    // 行列式 (列主序下按第一行展开)
    T determinant() const {
        // 第一行元素: m_mat[0][0], m_mat[1][0], m_mat[2][0]
        T cofactor00 = m_mat[1][1] * m_mat[2][2] - m_mat[2][1] * m_mat[1][2];
        T cofactor10 = -(m_mat[0][1] * m_mat[2][2] - m_mat[2][1] * m_mat[0][2]);
        T cofactor20 = m_mat[0][1] * m_mat[1][2] - m_mat[1][1] * m_mat[0][2];

        return m_mat[0][0] * cofactor00 + m_mat[1][0] * cofactor10 + m_mat[2][0] * cofactor20;
    }

    bool inverse(Mat3T& inv_mat, T tolerance = static_cast<T>(1e-6)) const {
        T det = determinant();
        if (std::abs(det) <= tolerance)
            return false;

        // 列主序下的伴随矩阵 (adjugate / det)
        // inv_mat[col][row] = cofactor[row][col] / det
        inv_mat[0][0] = m_mat[1][1] * m_mat[2][2] - m_mat[2][1] * m_mat[1][2];
        inv_mat[1][0] = -(m_mat[1][0] * m_mat[2][2] - m_mat[2][0] * m_mat[1][2]);
        inv_mat[2][0] = m_mat[1][0] * m_mat[2][1] - m_mat[2][0] * m_mat[1][1];
        inv_mat[0][1] = -(m_mat[0][1] * m_mat[2][2] - m_mat[2][1] * m_mat[0][2]);
        inv_mat[1][1] = m_mat[0][0] * m_mat[2][2] - m_mat[2][0] * m_mat[0][2];
        inv_mat[2][1] = -(m_mat[0][0] * m_mat[2][1] - m_mat[2][0] * m_mat[0][1]);
        inv_mat[0][2] = m_mat[0][1] * m_mat[1][2] - m_mat[1][1] * m_mat[0][2];
        inv_mat[1][2] = -(m_mat[0][0] * m_mat[1][2] - m_mat[1][0] * m_mat[0][2]);
        inv_mat[2][2] = m_mat[0][0] * m_mat[1][1] - m_mat[1][0] * m_mat[0][1];

        T inv_det = static_cast<T>(1) / det;
        for (size_t col = 0; col < 3; ++col) {
            for (size_t row = 0; row < 3; ++row)
                inv_mat.m_mat[col][row] *= inv_det;
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
