#ifndef HY_MATH_MAT4_HPP
#define HY_MATH_MAT4_HPP

#include "MathDef.hpp"
#include "Vec3.hpp"
#include "Vec4.hpp"
#include "Mat3.hpp"
#include <cmath>
#include <cassert>
#include <cstring>

namespace hyengine {
namespace math {

// 前置声明
template <typename T> class QuaternionT;

/**
 * @brief 4x4矩阵模板类
 * @tparam T 数值类型 (float, double等)
 * 
 * @note 使用列主序存储,符合OpenGL规范
 * 矩阵布局: m[row][col]
 * 
 * 变换顺序: Scale -> Rotate -> Translate (右手坐标系)
 */
template <typename T>
class Mat4T {
public:
    union {
        struct {
            T m00, m01, m02, m03;
            T m10, m11, m12, m13;
            T m20, m21, m22, m23;
            T m30, m31, m32, m33;
        };
        struct {
            Vec4T<T> row[4];
        };
        T m_mat[4][4];
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

    explicit Mat4T(const Vec4T<T>& row0, const Vec4T<T>& row1, 
                   const Vec4T<T>& row2, const Vec4T<T>& row3) {
        row[0] = row0;
        row[1] = row1;
        row[2] = row2;
        row[3] = row3;
    }

    // 访问器
    T* operator[](size_t row_index) {
        assert(row_index < 4);
        return m_mat[row_index];
    }

    const T* operator[](size_t row_index) const {
        assert(row_index < 4);
        return m_mat[row_index];
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

    Mat4T operator*(const Mat4T& m2) const {
        Mat4T r;
        for (size_t i = 0; i < 4; ++i) {
            for (size_t j = 0; j < 4; ++j) {
                r.m_mat[i][j] = m_mat[i][0] * m2.m_mat[0][j] +
                                m_mat[i][1] * m2.m_mat[1][j] +
                                m_mat[i][2] * m2.m_mat[2][j] +
                                m_mat[i][3] * m2.m_mat[3][j];
            }
        }
        return r;
    }

    // 矩阵 * Vec3 (w=1, 透视除法)
    Vec3T<T> operator*(const Vec3T<T>& v) const {
        T inv_w = static_cast<T>(1) / (m_mat[3][0] * v.x + m_mat[3][1] * v.y + 
                                       m_mat[3][2] * v.z + m_mat[3][3]);
        
        return Vec3T<T>(
            (m_mat[0][0] * v.x + m_mat[0][1] * v.y + m_mat[0][2] * v.z + m_mat[0][3]) * inv_w,
            (m_mat[1][0] * v.x + m_mat[1][1] * v.y + m_mat[1][2] * v.z + m_mat[1][3]) * inv_w,
            (m_mat[2][0] * v.x + m_mat[2][1] * v.y + m_mat[2][2] * v.z + m_mat[2][3]) * inv_w
        );
    }

    // 矩阵 * Vec4
    Vec4T<T> operator*(const Vec4T<T>& v) const {
        return Vec4T<T>(
            m_mat[0][0] * v.x + m_mat[0][1] * v.y + m_mat[0][2] * v.z + m_mat[0][3] * v.w,
            m_mat[1][0] * v.x + m_mat[1][1] * v.y + m_mat[1][2] * v.z + m_mat[1][3] * v.w,
            m_mat[2][0] * v.x + m_mat[2][1] * v.y + m_mat[2][2] * v.z + m_mat[2][3] * v.w,
            m_mat[3][0] * v.x + m_mat[3][1] * v.y + m_mat[3][2] * v.z + m_mat[3][3] * v.w
        );
    }

    // Vec4 * 矩阵
    friend Vec4T<T> operator*(const Vec4T<T>& v, const Mat4T& mat) {
        return Vec4T<T>(
            v.x * mat[0][0] + v.y * mat[1][0] + v.z * mat[2][0] + v.w * mat[3][0],
            v.x * mat[0][1] + v.y * mat[1][1] + v.z * mat[2][1] + v.w * mat[3][1],
            v.x * mat[0][2] + v.y * mat[1][2] + v.z * mat[2][2] + v.w * mat[3][2],
            v.x * mat[0][3] + v.y * mat[1][3] + v.z * mat[2][3] + v.w * mat[3][3]
        );
    }

    // 矩阵方法
    Mat4T transpose() const {
        return Mat4T(
            m_mat[0][0], m_mat[1][0], m_mat[2][0], m_mat[3][0],
            m_mat[0][1], m_mat[1][1], m_mat[2][1], m_mat[3][1],
            m_mat[0][2], m_mat[1][2], m_mat[2][2], m_mat[3][2],
            m_mat[0][3], m_mat[1][3], m_mat[2][3], m_mat[3][3]
        );
    }

    T getMinor(size_t r0, size_t r1, size_t r2, size_t c0, size_t c1, size_t c2) const {
        return m_mat[r0][c0] * (m_mat[r1][c1] * m_mat[r2][c2] - m_mat[r2][c1] * m_mat[r1][c2]) -
               m_mat[r0][c1] * (m_mat[r1][c0] * m_mat[r2][c2] - m_mat[r2][c0] * m_mat[r1][c2]) +
               m_mat[r0][c2] * (m_mat[r1][c0] * m_mat[r2][c1] - m_mat[r2][c0] * m_mat[r1][c1]);
    }

    T determinant() const {
        return m_mat[0][0] * getMinor(1, 2, 3, 1, 2, 3) -
               m_mat[0][1] * getMinor(1, 2, 3, 0, 2, 3) +
               m_mat[0][2] * getMinor(1, 2, 3, 0, 1, 3) -
               m_mat[0][3] * getMinor(1, 2, 3, 0, 1, 2);
    }

    Mat4T inverse() const {
        T m00 = m_mat[0][0], m01 = m_mat[0][1], m02 = m_mat[0][2], m03 = m_mat[0][3];
        T m10 = m_mat[1][0], m11 = m_mat[1][1], m12 = m_mat[1][2], m13 = m_mat[1][3];
        T m20 = m_mat[2][0], m21 = m_mat[2][1], m22 = m_mat[2][2], m23 = m_mat[2][3];
        T m30 = m_mat[3][0], m31 = m_mat[3][1], m32 = m_mat[3][2], m33 = m_mat[3][3];

        T v0 = m20 * m31 - m21 * m30;
        T v1 = m20 * m32 - m22 * m30;
        T v2 = m20 * m33 - m23 * m30;
        T v3 = m21 * m32 - m22 * m31;
        T v4 = m21 * m33 - m23 * m31;
        T v5 = m22 * m33 - m23 * m32;

        T t00 = +(v5 * m11 - v4 * m12 + v3 * m13);
        T t10 = -(v5 * m10 - v2 * m12 + v1 * m13);
        T t20 = +(v4 * m10 - v2 * m11 + v0 * m13);
        T t30 = -(v3 * m10 - v1 * m11 + v0 * m12);

        T invDet = static_cast<T>(1) / (t00 * m00 + t10 * m01 + t20 * m02 + t30 * m03);

        T d00 = t00 * invDet;
        T d10 = t10 * invDet;
        T d20 = t20 * invDet;
        T d30 = t30 * invDet;

        T d01 = -(v5 * m01 - v4 * m02 + v3 * m03) * invDet;
        T d11 = +(v5 * m00 - v2 * m02 + v1 * m03) * invDet;
        T d21 = -(v4 * m00 - v2 * m01 + v0 * m03) * invDet;
        T d31 = +(v3 * m00 - v1 * m01 + v0 * m02) * invDet;

        v0 = m10 * m31 - m11 * m30;
        v1 = m10 * m32 - m12 * m30;
        v2 = m10 * m33 - m13 * m30;
        v3 = m11 * m32 - m12 * m31;
        v4 = m11 * m33 - m13 * m31;
        v5 = m12 * m33 - m13 * m32;

        T d02 = +(v5 * m01 - v4 * m02 + v3 * m03) * invDet;
        T d12 = -(v5 * m00 - v2 * m02 + v1 * m03) * invDet;
        T d22 = +(v4 * m00 - v2 * m01 + v0 * m03) * invDet;
        T d32 = -(v3 * m00 - v1 * m01 + v0 * m02) * invDet;

        v0 = m21 * m10 - m20 * m11;
        v1 = m22 * m10 - m20 * m12;
        v2 = m23 * m10 - m20 * m13;
        v3 = m22 * m11 - m21 * m12;
        v4 = m23 * m11 - m21 * m13;
        v5 = m23 * m12 - m22 * m13;

        T d03 = -(v5 * m01 - v4 * m02 + v3 * m03) * invDet;
        T d13 = +(v5 * m00 - v2 * m02 + v1 * m03) * invDet;
        T d23 = -(v4 * m00 - v2 * m01 + v0 * m03) * invDet;
        T d33 = +(v3 * m00 - v1 * m01 + v0 * m02) * invDet;

        return Mat4T(d00, d01, d02, d03, d10, d11, d12, d13, 
                     d20, d21, d22, d23, d30, d31, d32, d33);
    }

    // 变换方法
    void setTrans(const Vec3T<T>& v) {
        m_mat[0][3] = v.x;
        m_mat[1][3] = v.y;
        m_mat[2][3] = v.z;
    }

    Vec3T<T> getTrans() const {
        return Vec3T<T>(m_mat[0][3], m_mat[1][3], m_mat[2][3]);
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

    bool isAffine() const {
        return m_mat[3][0] == 0 && m_mat[3][1] == 0 && m_mat[3][2] == 0 && m_mat[3][3] == 1;
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

    static Mat4T translate(const Vec3T<T>& v) {
        Mat4T m = identity();
        m.setTrans(v);
        return m;
    }

    static Mat4T scale(const Vec3T<T>& v) {
        return Mat4T(
            v.x, static_cast<T>(0), static_cast<T>(0), static_cast<T>(0),
            static_cast<T>(0), v.y, static_cast<T>(0), static_cast<T>(0),
            static_cast<T>(0), static_cast<T>(0), v.z, static_cast<T>(0),
            static_cast<T>(0), static_cast<T>(0), static_cast<T>(0), static_cast<T>(1)
        );
    }

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

    // 视图矩阵 - LookAt变换
    static Mat4T lookAt(const Vec3T<T>& eye_position, const Vec3T<T>& target_position, const Vec3T<T>& up_dir) {
        Vec3T<T> up = up_dir;
        up.normalise();

        Vec3T<T> f = target_position - eye_position;
        f.normalise();
        Vec3T<T> s = f.crossProduct(up);
        s.normalise();
        Vec3T<T> u = s.crossProduct(f);

        Mat4T view_mat = Mat4T::identity();

        view_mat.m_mat[0][0] = s.x;
        view_mat.m_mat[0][1] = s.y;
        view_mat.m_mat[0][2] = s.z;
        view_mat.m_mat[0][3] = -s.dotProduct(eye_position);
        view_mat.m_mat[1][0] = u.x;
        view_mat.m_mat[1][1] = u.y;
        view_mat.m_mat[1][2] = u.z;
        view_mat.m_mat[1][3] = -u.dotProduct(eye_position);
        view_mat.m_mat[2][0] = -f.x;
        view_mat.m_mat[2][1] = -f.y;
        view_mat.m_mat[2][2] = -f.z;
        view_mat.m_mat[2][3] = f.dotProduct(eye_position);
        return view_mat;
    }

    // 透视投影矩阵
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

    // 正交投影矩阵
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

        // 创建'uniform'正交投影矩阵
        // 深度范围 [-1,1], 右手坐标系
        //
        // [ A   0   0   C  ]
        // [ 0   B   0   D  ]
        // [ 0   0   q   qn ]
        // [ 0   0   0   1  ]

        Mat4T proj_matrix = Mat4T::zero();
        proj_matrix.m_mat[0][0] = A;
        proj_matrix.m_mat[0][3] = C;
        proj_matrix.m_mat[1][1] = B;
        proj_matrix.m_mat[1][3] = D;
        proj_matrix.m_mat[2][2] = q;
        proj_matrix.m_mat[2][3] = qn;
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
} // namespace hyengine

#endif // HY_MATH_MAT4_HPP
