#include "defintions.h"
#include <math.h>

#ifndef MATH_H
#define MATH_H


namespace glmath
{
    constexpr f32 PI = 3.14159265359f; //use std numerics instead
    bool approxEqual(f32 v1, f32 v2, f32 eps)
    {
        return (abs(v1 - v2) < eps);
    }

    struct Vec3
    {
        union
        {
            
            struct
            {
                f32 x,y,z;
            };
            struct
            {
                f32 r,g,b;
            };
            f32 data[3];
        };
        Vec3() = default;
        constexpr Vec3(f32 xp, f32 yp, f32 zp) : x(xp), y(yp), z(zp) {};

        constexpr Vec3(f32 v) : x(v), y(v), z(v) {};
    };

    Vec3 operator+(const Vec3 &lhs,const Vec3 &rhs )
    {
        return {lhs.x + rhs.x,
                lhs.y + rhs.y,
                lhs.z + rhs.z,
                };
    }
    void operator+=(Vec3 &v1,const Vec3 &v2 )
    {
        v1.x += v2.x;
        v1.y += v2.y;
        v1.z += v2.z;
    }
    Vec3 operator-(const Vec3 &lhs,const Vec3 &rhs )
    {
        return {lhs.x - rhs.x,
                lhs.y - rhs.y,
                lhs.z - rhs.z,
                };
    }
    Vec3 operator-(const Vec3 &vec )
    {
        return {-vec.x, -vec.y, -vec.z};
    }

    Vec3 operator*(const Vec3 &vec, f32 scale )
    {
        return {vec.x * scale, vec.y * scale, vec.z * scale};
    }
    Vec3 operator*(f32 scale, const Vec3 &vec)
    {
        return {vec.x * scale, vec.y * scale, vec.z * scale};
    }


    f32 dot(const Vec3 &v1, const Vec3 &v2)
    {
        return v1.x * v2.x +  v1.y * v2.y +  v1.z * v2.z;
    }
    f32 norm(const Vec3 &v)
    {
        return sqrt(dot(v,v));
    }

    Vec3 cross(const Vec3 &v1,const Vec3 &v2 )
    {
        Vec3 result;
        result.x = v1.y*v2.z - v1.z*v2.y;
        result.y = v1.z*v2.x - v1.x*v2.z;
        result.z = v1.x*v2.y - v1.y*v2.x;
        return result;
    }

    Vec3 operator/(const Vec3 &v, f32 q)
    {
        Vec3 result;
        result.x = v.x / q;
        result.y = v.y / q;
        result.z = v.z / q;
        return result;
    }
    
    struct Vec2
    {
        union
        {
            struct
            {
                f32 x,y;
            };
            struct
            {
                f32 u,v;
            };
            f32 data[2];
        };
        Vec2() = default;
        Vec2(f32 xp, f32 yp) : x(xp), y(yp) {};

        Vec2(f32 v) : x(v), y(v) {};
    };

    Vec2 operator+(const Vec2 &lhs,const Vec2 &rhs )
    {
        return {lhs.x + rhs.x,
                lhs.y + rhs.y,
                };
    }
    void operator+=(Vec2 &v1,const Vec2 &v2 )
    {
        v1.x += v2.x;
        v1.y += v2.y;
    }
    Vec2 operator-(const Vec2 &lhs,const Vec2 &rhs )
    {
        return {lhs.x - rhs.x,
                lhs.y - rhs.y,
                };
    }
    Vec2 operator-(const Vec2 &vec )
    {
        return {-vec.x, -vec.y};
    }

    Vec2 operator*(const Vec2 &vec, f32 scale )
    {
        return {vec.x * scale, vec.y * scale};
    }
    Vec2 operator*(f32 scale, const Vec2 &vec)
    {
        return {vec.x * scale, vec.y * scale};
    }

    f32 dot(const Vec2 &v1, const Vec2 &v2)
    {
        return v1.x * v2.x +  v1.y * v2.y;
    }
    f32 norm(const Vec2 &v)
    {
        return sqrt(dot(v,v));
    }

    Vec2 operator/(const Vec2 &v, f32 q)
    {
        Vec2 result;
        result.x = v.x / q;
        result.y = v.y / q;
        return result;
    }




    struct Vec4
    {
        union
        {
            
            struct
            {
                f32 x,y,z,w;
            };
            struct
            {
                f32 r,g,b,a;
            };
            f32 data[4];
        };
        Vec4() = default;
        Vec4(f32 xp, f32 yp, f32 zp, f32 wp) : x(xp), y(yp), z(zp), w(wp) {};
        Vec4(f32 v) : x(v), y(v), z(v), w(v) {};
    };
    
    Vec4 operator+(const Vec4 &lhs,const Vec4 &rhs )
    {
        return {lhs.x + rhs.x,
                lhs.y + rhs.y,
                lhs.z + rhs.z,
                lhs.w + rhs.w, 
                };
    }



    struct Mat3x3
    {
        f32 data[3][3];
        Mat3x3() = default;
        //Takes a row-major matrix and stores it as column-major
        Mat3x3(f32 m11, f32 m12, f32 m13,
               f32 m21, f32 m22, f32 m23,
               f32 m31, f32 m32, f32 m33
              )
            {
                data[0][0] = m11;
                data[0][1] = m21;
                data[0][2] = m31;

                data[1][0] = m12;
                data[1][1] = m22;
                data[1][2] = m32;

                data[2][0] = m13;
                data[2][1] = m23;
                data[2][2] = m33;
            };
    };

    //Angle is in radians
    Mat3x3 rotateY(f32 angle)
    {
        Mat3x3 result;
        result.data[0][0] = cos(angle);
        result.data[0][1] = 0.0;
        result.data[0][1] = -sin(angle);

        result.data[1][0] = 0.0;
        result.data[1][1] = 1.0;
        result.data[1][2] = 0.0;

        result.data[2][0] = sin(angle);
        result.data[2][1] = 0;
        result.data[2][2] = cos(angle);
        return result;
    }






    Vec3 operator*(const Mat3x3 &mat, const Vec3 & vec)
    {
        Vec3 result;
        result.x = mat.data[0][0] * vec.x + mat.data[1][0] * vec.y + mat.data[2][0] *vec.z;
        result.y = mat.data[0][1] * vec.x + mat.data[1][1] * vec.y + mat.data[2][1] *vec.z;
        result.z = mat.data[0][2] * vec.x + mat.data[1][2] * vec.y + mat.data[2][2] *vec.z;
        return result;
    }

    struct Mat4x4
    {

        union{
            f32 data[4][4];
            Vec4 columns[4];
        };
        Mat4x4() = default;

        //Takes a row-major matrix and stores it as column-major
        Mat4x4(f32 m11, f32 m12, f32 m13, f32 m14,
                f32 m21, f32 m22, f32 m23, f32 m24,
                f32 m31, f32 m32, f32 m33, f32 m34,
                f32 m41, f32 m42, f32 m43, f32 m44)
            {
                data[0][0] = m11;
                data[0][1] = m21;
                data[0][2] = m31;
                data[0][3] = m41;

                data[1][0] = m12;
                data[1][1] = m22;
                data[1][2] = m32;
                data[1][3] = m42;

                data[2][0] = m13;
                data[2][1] = m23;
                data[2][2] = m33;
                data[2][3] = m43;

                data[3][0] = m14;
                data[3][1] = m24;
                data[3][2] = m34;
                data[3][3] = m44;
            };
 
    };

    Mat4x4 transpose(const Mat4x4 &mat)
    {
        Mat4x4 result;
        result.data[0][0] = mat.data[0][0];
        result.data[0][1] = mat.data[1][0];
        result.data[0][2] = mat.data[2][0];
        result.data[0][3] = mat.data[3][0];

        result.data[1][0] = mat.data[0][1];
        result.data[1][1] = mat.data[1][1];
        result.data[1][2] = mat.data[2][1];
        result.data[1][3] = mat.data[3][1];

        result.data[2][0] = mat.data[0][2];
        result.data[2][1] = mat.data[1][2];
        result.data[2][2] = mat.data[2][2];
        result.data[2][3] = mat.data[3][2];

        result.data[3][0] = mat.data[0][3];
        result.data[3][1] = mat.data[1][3];
        result.data[3][2] = mat.data[2][3];
        result.data[3][3] = mat.data[3][3];
        return result;

    }

    // glmath::Mat4x4 adjoint(float3x3 mat)
    // {
    // mat = transpose(mat);
    // float3x3 cof;
    // // Use column major centred accesses

    //  cof[0][0] = mat[1][1]*mat[2][2] - mat[1][2]*mat[2][1];
    //  cof[1][0] = -mat[1][0]*mat[2][2] + mat[1][2]*mat[2][0];
    //  cof[2][0] = mat[1][0]*mat[2][1] - mat[1][1]*mat[2][0];

    //  cof[0][1] = -mat[0][1]*mat[2][2] + mat[0][2]*mat[2][1];
    //  cof[1][1] = mat[0][0]*mat[2][2] - mat[0][2]*mat[2][0];
    //  cof[2][1] = -mat[0][0]*mat[2][1] + mat[0][1]*mat[2][0];

    //  cof[0][2] = mat[0][1]*mat[1][2] - mat[0][2]*mat[1][1];
    //  cof[1][2] = -mat[0][0]*mat[1][2] + mat[0][2]*mat[1][0];
    //  cof[2][2] = mat[0][0]*mat[1][1] - mat[0][1]*mat[1][0];
    // return transpose(cof);
    // }




    Mat4x4 operator*(const Mat4x4 &m1, const Mat4x4 &m2)
    {
        Mat4x4 result;
        result.data[0][0] = m1.data[0][0] * m2.data[0][0] + m1.data[1][0] * m2.data[0][1] +
                            m1.data[2][0] * m2.data[0][2] + m1.data[3][0] * m2.data[0][3];
        result.data[1][0] = m1.data[0][0] * m2.data[1][0] + m1.data[1][0] * m2.data[1][1] +
                            m1.data[2][0] * m2.data[1][2] + m1.data[3][0] * m2.data[1][3];
        result.data[2][0] = m1.data[0][0] * m2.data[2][0] + m1.data[1][0] * m2.data[2][1] +
                            m1.data[2][0] * m2.data[2][2] + m1.data[3][0] * m2.data[2][3];                        
        result.data[3][0] = m1.data[0][0] * m2.data[3][0] + m1.data[1][0] * m2.data[3][1] +
                            m1.data[2][0] * m2.data[3][2] + m1.data[3][0] * m2.data[3][3];
        
        result.data[0][1] = m1.data[0][1] * m2.data[0][0] + m1.data[1][1] * m2.data[0][1] +
                            m1.data[2][1] * m2.data[0][2] + m1.data[3][1] * m2.data[0][3];
        result.data[1][1] = m1.data[0][1] * m2.data[1][0] + m1.data[1][1] * m2.data[1][1] +
                            m1.data[2][1] * m2.data[1][2] + m1.data[3][1] * m2.data[1][3];
        result.data[2][1] = m1.data[0][1] * m2.data[2][0] + m1.data[1][1] * m2.data[2][1] +
                            m1.data[2][1] * m2.data[2][2] + m1.data[3][1] * m2.data[2][3];                        
        result.data[3][1] = m1.data[0][1] * m2.data[3][0] + m1.data[1][1] * m2.data[3][1] +
                            m1.data[2][1] * m2.data[3][2] + m1.data[3][1] * m2.data[3][3];
        
        result.data[0][2] = m1.data[0][2] * m2.data[0][0] + m1.data[1][2] * m2.data[0][1] +
                            m1.data[2][2] * m2.data[0][2] + m1.data[3][2] * m2.data[0][3];
        result.data[1][2] = m1.data[0][2] * m2.data[1][0] + m1.data[1][2] * m2.data[1][1] +
                            m1.data[2][2] * m2.data[1][2] + m1.data[3][2] * m2.data[1][3];
        result.data[2][2] = m1.data[0][2] * m2.data[2][0] + m1.data[1][2] * m2.data[2][1] +
                            m1.data[2][2] * m2.data[2][2] + m1.data[3][2] * m2.data[2][3];                        
        result.data[3][2] = m1.data[0][2] * m2.data[3][0] + m1.data[1][2] * m2.data[3][1] +
                            m1.data[2][2] * m2.data[3][2] + m1.data[3][2] * m2.data[3][3];
        
        result.data[0][3] = m1.data[0][3] * m2.data[0][0] + m1.data[1][3] * m2.data[0][1] +
                            m1.data[2][3] * m2.data[0][2] + m1.data[3][3] * m2.data[0][3];
        result.data[1][3] = m1.data[0][3] * m2.data[1][0] + m1.data[1][3] * m2.data[1][1] +
                            m1.data[2][3] * m2.data[1][2] + m1.data[3][3] * m2.data[1][3];
        result.data[2][3] = m1.data[0][3] * m2.data[2][0] + m1.data[1][3] * m2.data[2][1] +
                            m1.data[2][3] * m2.data[2][2] + m1.data[3][3] * m2.data[2][3];                        
        result.data[3][3] = m1.data[0][3] * m2.data[3][0] + m1.data[1][3] * m2.data[3][1] +
                            m1.data[2][3] * m2.data[3][2] + m1.data[3][3] * m2.data[3][3];    
        return result;
    }

    Mat4x4 view(const Vec3 &forwardBasis, const Vec3 &rightBasis,const Vec3 &upBasis, const Vec3 &pos)
    {
        Mat4x4 result;

        result.data[0][0] = rightBasis.x;
        result.data[1][0] = rightBasis.y;
        result.data[2][0] = rightBasis.z;
        result.data[3][0] = -dot(pos,rightBasis);

        result.data[0][1] = upBasis.x;
        result.data[1][1] = upBasis.y;
        result.data[2][1] = upBasis.z;
        result.data[3][1] = -dot(pos,upBasis);

        result.data[0][2] = forwardBasis.x;
        result.data[1][2] = forwardBasis.y;
        result.data[2][2] = forwardBasis.z;
        result.data[3][2] = -dot(pos,forwardBasis);

        
        result.data[0][3] = 0;
        result.data[1][3] = 0;
        result.data[2][3] = 0;
        result.data[3][3] = 1;
        return result;
    }

    // LHS projection that looks down the +z axis stored in column-major

    // Takes the vertical FOV in radians

    Mat4x4 perspectiveProjection(f32 verticalFOV, f32  aspectRatio, f32 nearPlane, f32 farPlane)
    {
        f32 c = 1.0f / tan(verticalFOV / 2.0f);
        Mat4x4 result;
        result.data[0][0] = c / aspectRatio;
        result.data[1][0] = 0.0;
        result.data[2][0] = 0.0;
        result.data[3][0] = 0.0;

        result.data[0][1] = 0.0;
        result.data[1][1] = c;
        result.data[2][1] = 0.0;
        result.data[3][1] = 0.0;

        result.data[0][2] = 0.0;
        result.data[1][2] = 0.0;
        result.data[2][2] = (farPlane + nearPlane) / (farPlane - nearPlane);
        result.data[3][2] = -2 * farPlane * nearPlane / (farPlane - nearPlane);

        result.data[0][3] = 0.0;
        result.data[1][3] = 0.0;
        result.data[2][3] = 1.0;
        result.data[3][3] = 0.0;
        return result;
    }


    Vec4 operator*(const Mat4x4 &mat, const Vec4 & vec)
    {
        Vec4 result;
        result.x = mat.data[0][0] * vec.x + mat.data[1][0] * vec.y + mat.data[2][0] *vec.z + mat.data[3][0] *vec.w;
        result.y = mat.data[0][1] * vec.x + mat.data[1][1] * vec.y + mat.data[2][1] *vec.z + mat.data[3][1] *vec.w;
        result.z = mat.data[0][2] * vec.x + mat.data[1][2] * vec.y + mat.data[2][2] *vec.z + mat.data[3][2] *vec.w;
        result.w = mat.data[0][3] * vec.x + mat.data[1][3] * vec.y + mat.data[2][3] *vec.z + mat.data[3][3] *vec.w;
        return result;
    }


    Mat4x4 translate(f32 xTranslate, f32 yTranslate, f32 zTranslate)
    {
        Mat4x4 result;
        result.data[0][0] = 1.0;
        result.data[0][1] = 0.0;
        result.data[0][2] = 0.0;
        result.data[0][3] = 0.0;

        result.data[1][0] = 0.0;
        result.data[1][1] = 1.0;
        result.data[1][2] = 0.0;
        result.data[1][3] = 0.0;

        result.data[2][0] = 0.0;
        result.data[2][1] = 0.0;
        result.data[2][2] = 1.0;
        result.data[2][3] = 0.0;

        result.data[3][0] = xTranslate;
        result.data[3][1] = yTranslate;
        result.data[3][2] = zTranslate;
        result.data[3][3] = 1.0;
        return result;
    }
    Mat4x4 translate(glmath::Vec3 transform)
    {
        return translate(transform.x,transform.y,transform.z);
    }


    Mat4x4 scale(f32 xScale, f32 yScale, f32 zScale)
    {
        Mat4x4 result;
        result.data[0][0] = xScale;
        result.data[0][1] = 0.0;
        result.data[0][2] = 0.0;
        result.data[0][3] = 0.0;

        result.data[1][0] = 0.0;
        result.data[1][1] = yScale;
        result.data[1][2] = 0.0;
        result.data[1][3] = 0.0;

        result.data[2][0] = 0.0;
        result.data[2][1] = 0.0;
        result.data[2][2] = zScale;
        result.data[2][3] = 0.0;

        result.data[3][0] = 0.0f;
        result.data[3][1] = 0.0f;
        result.data[3][2] = 0.0f;
        result.data[3][3] = 1.0f;
        return result;
    }

    Mat4x4 scale(Vec3 transform)
    {
        return scale(transform.x, transform.y, transform.z);
    }




    Mat4x4 identity()
    {
        Mat4x4 result;
        result.data[0][0] = 1.0;
        result.data[1][0] = 0.0;
        result.data[2][0] = 0.0;
        result.data[3][0] = 0.0;

        result.data[0][1] = 0.0;
        result.data[1][1] = 1.0;
        result.data[2][1] = 0.0;
        result.data[3][1] = 0.0;

        result.data[0][2] = 0.0;
        result.data[1][2] = 0.0;
        result.data[2][2] = 1.0;
        result.data[3][2] = 0.0;

        result.data[0][3] = 0.0;
        result.data[1][3] = 0.0;
        result.data[2][3] = 0.0;
        result.data[3][3] = 1.0;
        return result;        
    }

    // angle must be in radians
    // axis must be a unit vector
    Mat4x4 rotateAroundAxis(f32 angle,const Vec3& u)
    {
        f32 c = cos(angle);
        f32 s = sin(angle);
        Mat4x4 result;

        result.data[0][0] = c + (1 - c) * u.x * u.x;
        result.data[1][0] = (1 - c) * u.x * u.y - s * u.z;
        result.data[2][0] = (1 - c) * u.x * u.z + s * u.y;
        result.data[3][0] = 0.0f;

        result.data[0][1] = (1 - c) * u.x *  u.y + s * u.z;
        result.data[1][1] = c + (1 - c) * u.y * u.y;
        result.data[2][1] = (1 - c) *u.y * u.z - s * u.x;
        result.data[3][1] = 0.0f;

        result.data[0][2] = (1 - c) * u.x *  u.z - s * u.y;
        result.data[1][2] =(1 - c) * u.y *  u.z + s * u.x;
        result.data[2][2] = c + (1 - c) * u.z*u.z;
        result.data[3][2] = 0.0f;

        result.data[0][3] = 0.0f;
        result.data[1][3] = 0.0f;
        result.data[2][3] = 0.0f;
        result.data[3][3] = 1.0f;
        
        return result;
    }


    struct Quaternion
    {
            struct
            {
                f32 x,y,z,w;
            };
        
        Quaternion(f32 xp, f32 yp,f32 zp, f32 wp) : x{xp},y{yp},z{zp},w{wp}{}
        Quaternion(glmath::Vec3 v,f32 s) : x{v.x}, y{v.y}, z{v.z}, w{s}{}
        Quaternion(){}
        f32 norm()
        {
            return sqrt(w*w + dot({x,y,z},{x,y,z}));
        }
        constexpr inline Vec3 getVectorComponent()
        {
            return {x,y,z};
        }
    };

    

    Quaternion operator*(const Quaternion &q1, const Quaternion &q2)
    {
        Quaternion result;
        result.x = q1.x * q2.w + q1.y * q2.z - q1.z * q2.y + q1.w * q2.x;
        result.y = -q1.x * q2.z + q1.y * q2.w +  q1.z * q2.x + q1.w * q2.y;
        result.z = q1.x * q2.y - q1.y * q2.x + q1.z * q2.w + q1.w * q2.z;
        result.w = -q1.x * q2.x - q1.y * q2.y - q1.z * q2.z + q1.w * q2.w;
        return result;
    }


    Mat4x4 quaternionToMatrix(const Quaternion &q)
    {
        Mat4x4 result;
        result.data[0][0] = 1.0f - 2.0f * (q.y * q.y + q.z * q.z);
        result.data[1][0] = 2.0f * (q.x * q.y - q.w * q.z);
        result.data[2][0] = 2.0f * (q.x * q.z + q.w * q.y);
        result.data[3][0] = 0.0f;

        result.data[0][1] = 2.0f * (q.x * q.y + q.w * q.z);
        result.data[1][1] = 1.0f - 2.0f * (q.x * q.x + q.z * q.z);
        result.data[2][1] = 2.0f * (q.y * q.z - q.w * q.x);
        result.data[3][1] = 0.0f;

        result.data[0][2] = 2.0f * (q.x * q.z - q.w * q.y);
        result.data[1][2] = 2.0f * (q.y * q.z + q.w * q.x);
        result.data[2][2] = 1.0f - 2.0f * (q.x * q.x + q.y * q.y);
        result.data[3][2] = 0.0f;

        result.data[0][3] = 0.0f;
        result.data[1][3] = 0.0f;
        result.data[2][3] = 0.0f;
        result.data[3][3] = 1.0f;

        return result;
    }

    Quaternion eulerAngleToQuaternion(f32 roll, f32 yaw, f32 pitch)
    {
        f32 cr = cos(roll / 2.0f);
        f32 sr = sin(roll / 2.0f);
        
        f32 cy = cos(yaw / 2.0f);
        f32 sy = sin(yaw / 2.0f);

        f32 cp = cos(pitch / 2.0f);
        f32 sp = sin(pitch / 2.0f);

        Quaternion result;

        result.x = -cp * sy * sr + sp * cy * cr;

        result.y = cp * cy * sr - sp * sy *cr;

        result.z = sp * cy * sr + cp * sy *cr;

        result.w = sp * sy * sr + cp * cy *cr;

        return result;

    }


}

#endif //MATH_H