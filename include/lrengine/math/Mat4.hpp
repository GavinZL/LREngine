#ifndef HY_MATH_MAT4_HPP
#define HY_MATH_MAT4_HPP

#include "MathDef.hpp"
#include "Vec3.hpp"
#include "Vec4.hpp"
#include "Mat3.hpp"
#include <cmath>
#include <cassert>
#include <cstring>

namespace lrengine {
namespace math {

// 前置声明
template <typename T> class QuaternionT;

/**
 * @brief 4x4矩阵模板类
 * @tparam T 数值类型 (float, double等)
 * 
 * @note 使用列主序存储,符合OpenGL规范
 * 矩阵布局: m_mat[col][row], 内存连续按列排列
 * 
 * 矩阵内存布局:
 * | m00 m10 m20 m30 |   内存顺序: m00,m01,m02,m03, m10,m11,m12,m13, m20,m21,m22,m23, m30,m31,m32,m33
 * | m01 m11 m21 m31 |   即: col0, col1, col2, col3
 * | m02 m12 m22 m32 |
 * | m03 m13 m23 m33 |
 * 
 * 变换顺序: Scale -> Rotate -> Translate (右手坐标系)
 */
template <typename T>
class Mat4T {
public:
    union {
        struct {
            // 列主序: 按列存储
            T m00, m01, m02, m03;  // 第0列
            T m10, m11, m12, m13;  // 第1列
            T m20, m21, m22, m23;  // 第2列
            T m30, m31, m32, m33;  // 第3列
        };
        struct {
            Vec4T<T> col[4];  // 改为col表示列
        };
        T m_mat[4][4];  // m_mat[col][row]
        T m[16];
    };

public:
    // 构造函数
    Mat4T() {
        *this = identity();
    }

    explicit Mat4T(const T* float_array, uint32_t count = 16) {
        assert(count == 16);
        std::memcpy(&m[0], float_array, sizeof(T) * count);
    }

    explicit Mat4T(T m00, T m01, T m02, T m03,
                   T m10, T m11, T m12, T m13,
                   T m20, T m21, T m22, T m23,
                   T m30, T m31, T m32, T m33) {
        m_mat[0][0] = m00; m_mat[0][1] = m01; m_mat[0][2] = m02; m_mat[0][3] = m03;
        m_mat[1][0] = m10; m_mat[1][1] = m11; m_mat[1][2] = m12; m_mat[1][3] = m13;
        m_mat[2][0] = m20; m_mat[2][1] = m21; m_mat[2][2] = m22; m_mat[2][3] = m23;
        m_mat[3][0] = m30; m_mat[3][1] = m31; m_mat[3][2] = m32; m_mat[3][3] = m33;
    }

    // 从列向量构造
    explicit Mat4T(const Vec4T<T>& col0, const Vec4T<T>& col1, 
                   const Vec4T<T>& col2, const Vec4T<T>& col3) {
        col[0] = col0;
        col[1] = col1;
        col[2] = col2;
        col[3] = col3;
    }

    // 访问器 - 返回列向量
    T* operator[](size_t col_index) {
        assert(col_index < 4);
        return m_mat[col_index];
    }

    const T* operator[](size_t col_index) const {
        assert(col_index < 4);
        return m_mat[col_index];
    }

    const T* data() const {
        return &m_mat[0][0];
    }

    // 比较运算符
    bool operator==(const Mat4T& rhs) const {
        for (size_t i = 0; i < 4; ++i) {
            for (size_t j = 0; j < 4; ++j) {
                if (m_mat[i][j] != rhs.m_mat[i][j])
                    return false;
            }
        }
        return true;
    }

    bool operator!=(const Mat4T& rhs) const {
        return !operator==(rhs);
    }

    // 算术运算符
    Mat4T operator+(const Mat4T& rhs) const {
        Mat4T r;
        for (size_t i = 0; i < 4; ++i) {
            for (size_t j = 0; j < 4; ++j) {
                r.m_mat[i][j] = m_mat[i][j] + rhs.m_mat[i][j];
            }
        }
        return r;
    }

    Mat4T operator-(const Mat4T& rhs) const {
        Mat4T r;
        for (size_t i = 0; i < 4; ++i) {
            for (size_t j = 0; j < 4; ++j) {
                r.m_mat[i][j] = m_mat[i][j] - rhs.m_mat[i][j];
            }
        }
        return r;
    }

    Mat4T operator*(T scalar) const {
        Mat4T r;
        for (size_t i = 0; i < 4; ++i) {
            for (size_t j = 0; j < 4; ++j) {
                r.m_mat[i][j] = m_mat[i][j] * scalar;
            }
        }
        return r;
    }

    // 矩阵乘法 (列主序: C = A * B, C[col][row] = sum(A[k][row] * B[col][k]))
    Mat4T operator*(const Mat4T& m2) const {
        Mat4T r;
        for (size_t col = 0; col < 4; ++col) {
            for (size_t row = 0; row < 4; ++row) {
                r.m_mat[col][row] = m_mat[0][row] * m2.m_mat[col][0] +
                                    m_mat[1][row] * m2.m_mat[col][1] +
                                    m_mat[2][row] * m2.m_mat[col][2] +
                                    m_mat[3][row] * m2.m_mat[col][3];
            }
        }
        return r;
    }

    // 矩阵 * Vec3 (w=1, 透视除法) - v' = M * v
    Vec3T<T> operator*(const Vec3T<T>& v) const {
        T inv_w = static_cast<T>(1) / (m_mat[0][3] * v.x + m_mat[1][3] * v.y + 
                                       m_mat[2][3] * v.z + m_mat[3][3]);
        
        return Vec3T<T>(
            (m_mat[0][0] * v.x + m_mat[1][0] * v.y + m_mat[2][0] * v.z + m_mat[3][0]) * inv_w,
            (m_mat[0][1] * v.x + m_mat[1][1] * v.y + m_mat[2][1] * v.z + m_mat[3][1]) * inv_w,
            (m_mat[0][2] * v.x + m_mat[1][2] * v.y + m_mat[2][2] * v.z + m_mat[3][2]) * inv_w
        );
    }

    // 矩阵 * Vec4 - v' = M * v
    Vec4T<T> operator*(const Vec4T<T>& v) const {
        return Vec4T<T>(
            m_mat[0][0] * v.x + m_mat[1][0] * v.y + m_mat[2][0] * v.z + m_mat[3][0] * v.w,
            m_mat[0][1] * v.x + m_mat[1][1] * v.y + m_mat[2][1] * v.z + m_mat[3][1] * v.w,
            m_mat[0][2] * v.x + m_mat[1][2] * v.y + m_mat[2][2] * v.z + m_mat[3][2] * v.w,
            m_mat[0][3] * v.x + m_mat[1][3] * v.y + m_mat[2][3] * v.z + m_mat[3][3] * v.w
        );
    }

    // Vec4 * 矩阵 - v' = v * M = M^T * v
    friend Vec4T<T> operator*(const Vec4T<T>& v, const Mat4T& mat) {
        return Vec4T<T>(
            v.x * mat[0][0] + v.y * mat[0][1] + v.z * mat[0][2] + v.w * mat[0][3],
            v.x * mat[1][0] + v.y * mat[1][1] + v.z * mat[1][2] + v.w * mat[1][3],
            v.x * mat[2][0] + v.y * mat[2][1] + v.z * mat[2][2] + v.w * mat[2][3],
            v.x * mat[3][0] + v.y * mat[3][1] + v.z * mat[3][2] + v.w * mat[3][3]
        );
    }

    // 矩阵方法
    Mat4T transpose() const {
        return Mat4T(
            m_mat[0][0], m_mat[0][1], m_mat[0][2], m_mat[0][3],
            m_mat[1][0], m_mat[1][1], m_mat[1][2], m_mat[1][3],
            m_mat[2][0], m_mat[2][1], m_mat[2][2], m_mat[2][3],
            m_mat[3][0], m_mat[3][1], m_mat[3][2], m_mat[3][3]
        );
    }

    // 列主序下的小行列式
    T getMinor(size_t c0, size_t c1, size_t c2, size_t r0, size_t r1, size_t r2) const {
        return m_mat[c0][r0] * (m_mat[c1][r1] * m_mat[c2][r2] - m_mat[c2][r1] * m_mat[c1][r2]) -
               m_mat[c1][r0] * (m_mat[c0][r1] * m_mat[c2][r2] - m_mat[c2][r1] * m_mat[c0][r2]) +
               m_mat[c2][r0] * (m_mat[c0][r1] * m_mat[c1][r2] - m_mat[c1][r1] * m_mat[c0][r2]);
    }

    T determinant() const {
        return m_mat[0][0] * getMinor(1, 2, 3, 1, 2, 3) -
               m_mat[1][0] * getMinor(0, 2, 3, 1, 2, 3) +
               m_mat[2][0] * getMinor(0, 1, 3, 1, 2, 3) -
               m_mat[3][0] * getMinor(0, 1, 2, 1, 2, 3);
    }

    Mat4T inverse() const {
        // 列主序下的矩阵元素
        T m00 = m_mat[0][0], m01 = m_mat[0][1], m02 = m_mat[0][2], m03 = m_mat[0][3];
        T m10 = m_mat[1][0], m11 = m_mat[1][1], m12 = m_mat[1][2], m13 = m_mat[1][3];
        T m20 = m_mat[2][0], m21 = m_mat[2][1], m22 = m_mat[2][2], m23 = m_mat[2][3];
        T m30 = m_mat[3][0], m31 = m_mat[3][1], m32 = m_mat[3][2], m33 = m_mat[3][3];

        T v0 = m02 * m13 - m03 * m12;
        T v1 = m02 * m23 - m03 * m22;
        T v2 = m02 * m33 - m03 * m32;
        T v3 = m12 * m23 - m13 * m22;
        T v4 = m12 * m33 - m13 * m32;
        T v5 = m22 * m33 - m23 * m32;

        T t00 = +(v5 * m11 - v4 * m21 + v3 * m31);
        T t10 = -(v5 * m01 - v2 * m21 + v1 * m31);
        T t20 = +(v4 * m01 - v2 * m11 + v0 * m31);
        T t30 = -(v3 * m01 - v1 * m11 + v0 * m21);

        T invDet = static_cast<T>(1) / (t00 * m00 + t10 * m10 + t20 * m20 + t30 * m30);

        T d00 = t00 * invDet;
        T d01 = t10 * invDet;
        T d02 = t20 * invDet;
        T d03 = t30 * invDet;

        T d10 = -(v5 * m10 - v4 * m20 + v3 * m30) * invDet;
        T d11 = +(v5 * m00 - v2 * m20 + v1 * m30) * invDet;
        T d12 = -(v4 * m00 - v2 * m10 + v0 * m30) * invDet;
        T d13 = +(v3 * m00 - v1 * m10 + v0 * m20) * invDet;

        v0 = m01 * m10 - m00 * m11;
        v1 = m01 * m20 - m00 * m21;
        v2 = m01 * m30 - m00 * m31;
        v3 = m11 * m20 - m10 * m21;
        v4 = m11 * m30 - m10 * m31;
        v5 = m21 * m30 - m20 * m31;

        T d20 = +(v5 * m13 - v4 * m23 + v3 * m33) * invDet;
        T d21 = -(v5 * m03 - v2 * m23 + v1 * m33) * invDet;
        T d22 = +(v4 * m03 - v2 * m13 + v0 * m33) * invDet;
        T d23 = -(v3 * m03 - v1 * m13 + v0 * m23) * invDet;

        T d30 = -(v5 * m12 - v4 * m22 + v3 * m32) * invDet;
        T d31 = +(v5 * m02 - v2 * m22 + v1 * m32) * invDet;
        T d32 = -(v4 * m02 - v2 * m12 + v0 * m32) * invDet;
        T d33 = +(v3 * m02 - v1 * m12 + v0 * m22) * invDet;

        // 列主序: m_mat[col][row]
        return Mat4T(d00, d01, d02, d03, d10, d11, d12, d13, 
                     d20, d21, d22, d23, d30, d31, d32, d33);
    }

    // 变换方法 - 列主序下平移在第3列
    void setTrans(const Vec3T<T>& v) {
        m_mat[3][0] = v.x;
        m_mat[3][1] = v.y;
        m_mat[3][2] = v.z;
    }

    Vec3T<T> getTrans() const {
        return Vec3T<T>(m_mat[3][0], m_mat[3][1], m_mat[3][2]);
    }

    void setScale(const Vec3T<T>& v) {
        m_mat[0][0] = v.x;
        m_mat[1][1] = v.y;
        m_mat[2][2] = v.z;
    }

    void extract3x3Matrix(Mat3T<T>& m3x3) const {
        m3x3.m_mat[0][0] = m_mat[0][0]; m3x3.m_mat[0][1] = m_mat[0][1]; m3x3.m_mat[0][2] = m_mat[0][2];
        m3x3.m_mat[1][0] = m_mat[1][0]; m3x3.m_mat[1][1] = m_mat[1][1]; m3x3.m_mat[1][2] = m_mat[1][2];
        m3x3.m_mat[2][0] = m_mat[2][0]; m3x3.m_mat[2][1] = m_mat[2][1]; m3x3.m_mat[2][2] = m_mat[2][2];
    }

    // 列主序下第3行应为(0,0,0,1)
    bool isAffine() const {
        return m_mat[0][3] == 0 && m_mat[1][3] == 0 && m_mat[2][3] == 0 && m_mat[3][3] == 1;
    }

    // 静态工厂方法
    static Mat4T identity() {
        return Mat4T(
            static_cast<T>(1), static_cast<T>(0), static_cast<T>(0), static_cast<T>(0),
            static_cast<T>(0), static_cast<T>(1), static_cast<T>(0), static_cast<T>(0),
            static_cast<T>(0), static_cast<T>(0), static_cast<T>(1), static_cast<T>(0),
            static_cast<T>(0), static_cast<T>(0), static_cast<T>(0), static_cast<T>(1)
        );
    }

    static Mat4T zero() {
        return Mat4T(
            static_cast<T>(0), static_cast<T>(0), static_cast<T>(0), static_cast<T>(0),
            static_cast<T>(0), static_cast<T>(0), static_cast<T>(0), static_cast<T>(0),
            static_cast<T>(0), static_cast<T>(0), static_cast<T>(0), static_cast<T>(0),
            static_cast<T>(0), static_cast<T>(0), static_cast<T>(0), static_cast<T>(0)
        );
    }

    // 列主序平移矩阵 - 平移在第3列
    static Mat4T translate(const Vec3T<T>& v) {
        Mat4T m = identity();
        m.m_mat[3][0] = v.x;
        m.m_mat[3][1] = v.y;
        m.m_mat[3][2] = v.z;
        return m;
    }

    // 列主序缩放矩阵
    static Mat4T scale(const Vec3T<T>& v) {
        return Mat4T(
            v.x, static_cast<T>(0), static_cast<T>(0), static_cast<T>(0),
            static_cast<T>(0), v.y, static_cast<T>(0), static_cast<T>(0),
            static_cast<T>(0), static_cast<T>(0), v.z, static_cast<T>(0),
            static_cast<T>(0), static_cast<T>(0), static_cast<T>(0), static_cast<T>(1)
        );
    }

    // 列主序 X轴旋转矩阵
    // | 1   0    0   0 |
    // | 0   c   -s   0 |
    // | 0   s    c   0 |
    // | 0   0    0   1 |
    static Mat4T rotateX(T radian) {
        T c = std::cos(radian);
        T s = std::sin(radian);
        return Mat4T(
            static_cast<T>(1), static_cast<T>(0), static_cast<T>(0), static_cast<T>(0),
            static_cast<T>(0), c, s, static_cast<T>(0),
            static_cast<T>(0), -s, c, static_cast<T>(0),
            static_cast<T>(0), static_cast<T>(0), static_cast<T>(0), static_cast<T>(1)
        );
    }

    // 列主序 Y轴旋转矩阵
    // | c   0   s   0 |
    // | 0   1   0   0 |
    // |-s   0   c   0 |
    // | 0   0   0   1 |
    static Mat4T rotateY(T radian) {
        T c = std::cos(radian);
        T s = std::sin(radian);
        return Mat4T(
            c, static_cast<T>(0), -s, static_cast<T>(0),
            static_cast<T>(0), static_cast<T>(1), static_cast<T>(0), static_cast<T>(0),
            s, static_cast<T>(0), c, static_cast<T>(0),
            static_cast<T>(0), static_cast<T>(0), static_cast<T>(0), static_cast<T>(1)
        );
    }

    // 列主序 Z轴旋转矩阵
    // | c  -s   0   0 |
    // | s   c   0   0 |
    // | 0   0   1   0 |
    // | 0   0   0   1 |
    static Mat4T rotateZ(T radian) {
        T c = std::cos(radian);
        T s = std::sin(radian);
        return Mat4T(
            c, s, static_cast<T>(0), static_cast<T>(0),
            -s, c, static_cast<T>(0), static_cast<T>(0),
            static_cast<T>(0), static_cast<T>(0), static_cast<T>(1), static_cast<T>(0),
            static_cast<T>(0), static_cast<T>(0), static_cast<T>(0), static_cast<T>(1)
        );
    }

    // 视图矩阵 - LookAt变换 (列主序)
    // 结果矩阵布局:
    // | sx  sy  sz  -s·eye |
    // | ux  uy  uz  -u·eye |
    // |-fx -fy -fz   f·eye |
    // |  0   0   0     1   |
    static Mat4T lookAt(const Vec3T<T>& eye_position, const Vec3T<T>& target_position, const Vec3T<T>& up_dir) {
        Vec3T<T> up = up_dir;
        up.normalise();

        Vec3T<T> f = target_position - eye_position;
        f.normalise();
        Vec3T<T> s = f.crossProduct(up);
        s.normalise();
        Vec3T<T> u = s.crossProduct(f);

        Mat4T view_mat = Mat4T::identity();

        // 列主序: m_mat[col][row]
        // 第0列 (s向量)
        view_mat.m_mat[0][0] = s.x;
        view_mat.m_mat[0][1] = u.x;
        view_mat.m_mat[0][2] = -f.x;
        // 第1列 (u向量)
        view_mat.m_mat[1][0] = s.y;
        view_mat.m_mat[1][1] = u.y;
        view_mat.m_mat[1][2] = -f.y;
        // 第2列 (-f向量)
        view_mat.m_mat[2][0] = s.z;
        view_mat.m_mat[2][1] = u.z;
        view_mat.m_mat[2][2] = -f.z;
        // 第3列 (平移)
        view_mat.m_mat[3][0] = -s.dotProduct(eye_position);
        view_mat.m_mat[3][1] = -u.dotProduct(eye_position);
        view_mat.m_mat[3][2] = f.dotProduct(eye_position);
        return view_mat;
    }

    // 透视投影矩阵 (列主序, OpenGL NDC z∈[-1,1])
    // | f/a   0    0    0  |
    // |  0    f    0    0  |
    // |  0    0    A    B  |
    // |  0    0   -1    0  |
    static Mat4T perspective(T fovy, T aspect, T znear, T zfar) {
        T tan_half_fovy = std::tan(fovy / static_cast<T>(2));

        Mat4T ret = Mat4T::zero();
        ret.m_mat[0][0] = static_cast<T>(1) / (aspect * tan_half_fovy);
        ret.m_mat[1][1] = static_cast<T>(1) / tan_half_fovy;
        ret.m_mat[2][2] = (zfar + znear) / (znear - zfar);
        ret.m_mat[2][3] = static_cast<T>(-1);
        ret.m_mat[3][2] = static_cast<T>(2) * zfar * znear / (znear - zfar);

        return ret;
    }

    // 正交投影矩阵 (列主序)
    // | A   0   0   C  |
    // | 0   B   0   D  |
    // | 0   0   q   qn |
    // | 0   0   0   1  |
    static Mat4T ortho(T left, T right, T bottom, T top, T znear, T zfar) {
        T inv_width = static_cast<T>(1) / (right - left);
        T inv_height = static_cast<T>(1) / (top - bottom);
        T inv_distance = static_cast<T>(1) / (zfar - znear);

        T A = static_cast<T>(2) * inv_width;
        T B = static_cast<T>(2) * inv_height;
        T C = -(right + left) * inv_width;
        T D = -(top + bottom) * inv_height;
        T q = static_cast<T>(-2) * inv_distance;
        T qn = -(zfar + znear) * inv_distance;

        // 列主序正交投影矩阵
        // 深度范围 [-1,1], 右手坐标系
        Mat4T proj_matrix = Mat4T::zero();
        proj_matrix.m_mat[0][0] = A;
        proj_matrix.m_mat[1][1] = B;
        proj_matrix.m_mat[2][2] = q;
        proj_matrix.m_mat[3][0] = C;
        proj_matrix.m_mat[3][1] = D;
        proj_matrix.m_mat[3][2] = qn;
        proj_matrix.m_mat[3][3] = static_cast<T>(1);

        return proj_matrix;
    }

    // 静态常量
    static const Mat4T ZERO;
    static const Mat4T IDENTITY;
};

// 静态常量定义
template <typename T>
const Mat4T<T> Mat4T<T>::ZERO = Mat4T<T>::zero();
template <typename T>
const Mat4T<T> Mat4T<T>::IDENTITY = Mat4T<T>::identity();

} // namespace math
} // namespace lrengine

#endif // HY_MATH_MAT4_HPP
