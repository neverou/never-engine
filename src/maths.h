#pragma once

#include "core.h"

// Only for INFINITY constant
#include <math.h>
constexpr float PI = 3.14159265358979;
constexpr float Infinity = INFINITY;

struct Vec2 {
    float x;
    float y;
};

struct Vec3 {
    float x;
    float y;
    float z;
};

struct Vec4 {
    float x;
    float y;
    float z;
    float w;
};


// ~Todo: come up with better names for these?

// also maybe:
// either do Vec or CreateVector (latter to be consistent with matrices)
// or maybe lets switch CreateMatrix() to Mat? idk

Vec2 v2(float x, float y);
Vec3 v3(float x, float y, float z);
Vec4 v4(float x, float y, float z, float w);

Vec3 v3(Vec2 xy, float z);
Vec3 v3(float x, Vec2 yz);

Vec4 v4(Vec2 xy, float z, float w);
Vec4 v4(float x, float y, Vec2 zw);
Vec4 v4(float x, Vec2 yz, float w);
Vec4 v4(Vec2 xy, Vec2 zw);
Vec4 v4(Vec3 xyz, float w);
Vec4 v4(float x, Vec3 yzw);


Vec2 v2(float x);
Vec3 v3(float x);
Vec4 v4(float x);

// Downscale conversions
// 2
Vec2 v2(Vec3 xy);
Vec2 v2(Vec4 xy);
//3
Vec3 v3(Vec4 xyz);




Vec2 Add(Vec2 a, Vec2 b);
Vec3 Add(Vec3 a, Vec3 b);
Vec4 Add(Vec4 a, Vec4 b);

Vec2 operator+(Vec2 a, Vec2 b);
Vec3 operator+(Vec3 a, Vec3 b);
Vec4 operator+(Vec4 a, Vec4 b);

inline Vec2 operator+=(Vec2& a, Vec2 b) { a = a + b; return a; }
inline Vec3 operator+=(Vec3& a, Vec3 b) { a = a + b; return a; }
inline Vec4 operator+=(Vec4& a, Vec4 b) { a = a + b; return a; }



Vec2 Sub(Vec2 a, Vec2 b);
Vec3 Sub(Vec3 a, Vec3 b);
Vec4 Sub(Vec4 a, Vec4 b);

Vec2 operator-(Vec2 a, Vec2 b);
Vec3 operator-(Vec3 a, Vec3 b);
Vec4 operator-(Vec4 a, Vec4 b);

inline Vec2 operator-=(Vec2& a, Vec2 b) { a = a - b; return a; }
inline Vec3 operator-=(Vec3& a, Vec3 b) { a = a - b; return a; }
inline Vec4 operator-=(Vec4& a, Vec4 b) { a = a - b; return a; }

Vec2 Mul(Vec2 a, Vec2 b);
Vec3 Mul(Vec3 a, Vec3 b);
Vec4 Mul(Vec4 a, Vec4 b);

Vec2 operator*(Vec2 a, Vec2 b);
Vec3 operator*(Vec3 a, Vec3 b);
Vec4 operator*(Vec4 a, Vec4 b);

inline Vec2 operator*=(Vec2& a, Vec2 b) { a = a * b; return a; }
inline Vec3 operator*=(Vec3& a, Vec3 b) { a = a * b; return a; }
inline Vec4 operator*=(Vec4& a, Vec4 b) { a = a * b; return a; }


Vec2 Div(Vec2 a, Vec2 b);
Vec3 Div(Vec3 a, Vec3 b);
Vec4 Div(Vec4 a, Vec4 b);

Vec2 operator/(Vec2 a, Vec2 b);
Vec3 operator/(Vec3 a, Vec3 b);
Vec4 operator/(Vec4 a, Vec4 b);

inline Vec2 operator/=(Vec2& a, Vec2 b) { a = a / b; return a; }
inline Vec3 operator/=(Vec3& a, Vec3 b) { a = a / b; return a; }
inline Vec4 operator/=(Vec4& a, Vec4 b) { a = a / b; return a; }





float Dot(Vec2 a, Vec2 b);
float Dot(Vec3 a, Vec3 b);
float Dot(Vec4 a, Vec4 b);

Vec3 Cross(Vec3 lhs, Vec3 rhs);

float Length(Vec2 a);
float Length(Vec3 a);
float Length(Vec4 a);

Vec2 Normalize(Vec2 a);
Vec3 Normalize(Vec3 a);
Vec4 Normalize(Vec4 a);

bool Equal(Vec2 a, Vec2 b);
bool Equal(Vec3 a, Vec3 b);
bool Equal(Vec4 a, Vec4 b);


////

struct Mat4 {
    union { 
        float m[4 * 4];
        Vec4 rows[4];
    };
};

Mat4 CreateMatrix(float trace);

Mat4 CreateMatrix(float m00, float m10, float m20, float m30,
                  float m01, float m11, float m21, float m31,
                  float m02, float m12, float m22, float m32,
                  float m03, float m13, float m23, float m33); 


Mat4 PerspectiveMatrix(float fov, float aspect, float start, float end);
Mat4 OrthoMatrix(float left, float right, float bottom, float top, float start, float end);

Mat4 LookAtMatrix(Vec3 dir, Vec3 up);

Mat4 TranslationMatrix(Vec3 translation);
// Mat4 RotationMatrix(); // ~Todo: rotors. bye bye quaternion you "pizzacrap"
Mat4 RotationMatrixAxisAngle(Vec3 axis, float angle);
Mat4 ScaleMatrix(Vec3 scale);

Mat4 Mul(Mat4 a, Mat4 b);
Mat4 Transpose(Mat4 mat);
Mat4 Inverse(Mat4 mat);

Vec4 Mul(Vec4 v, Mat4 mat);
Vec3 Mul(Vec3 v, float w, Mat4 mat);

Vec3 Project(Vec3 v, Mat4 projectionMatrix);
Vec3 Unproject(Vec3 ndc, Mat4 projectionMatrix);


struct Xform {
	Vec3 position;
	Mat4 rotation; // This should be a rotor
	Vec3 scale;
};

Xform CreateXform();
Mat4 XformMatrix(Xform xform);


Vec3 OrientForward(Mat4 mat);
Vec3 OrientRight(Mat4 mat);
Vec3 OrientUp(Mat4 mat);

////

struct Rect {
    u32 x;
    u32 y;
    u32 width;
    u32 height;
};


// 

float Sin(float x);
float Cos(float x);
float Tan(float x);
float Atan(float x);
float Acos(float x);
float Abs(float x);

float Sqrt(float x);

float Min(float x, float y);
float Max(float x, float y);
float Clamp(float v, float min, float max);

float Floor(float x);
float Fract(float x);

float Lerp(float a, float b, float t);


u64 PowU64(u64 base, u64 exponent);
float PowF(float base, float exponent);