#include "maths.h"

#include <math.h>

#include "error.h"
#include "core.h"
#include "logger.h"
#include <string.h> // ~Temp for memcpy

Vec2 v2(float x, float y) { return Vec2 { x, y }; }
Vec3 v3(float x, float y, float z) { return Vec3 { x, y, z }; }
Vec4 v4(float x, float y, float z, float w) { return Vec4 { x, y, z, w }; }

Vec2 v2(float x) { return Vec2 { x, x }; }
Vec3 v3(float x) { return Vec3 { x, x, x }; }
Vec4 v4(float x) { return Vec4 { x, x, x, x }; }


Vec3 v3(Vec2 xy, float z) { return Vec3{ xy.x, xy.y, z }; }
Vec3 v3(float x, Vec2 yz) { return Vec3{ x, yz.x, yz.y }; }

Vec4 v4(Vec2 xy, float z, float w) { return Vec4 { xy.x, xy.y, z, w }; }
Vec4 v4(float x, float y, Vec2 zw) { return Vec4 { x, y, zw.x, zw.y }; }
Vec4 v4(float x, Vec2 yz, float w) { return Vec4 { x, yz.x, yz.y, w }; }
Vec4 v4(Vec2 xy, Vec2 zw) { return Vec4 { xy.x, xy.y, zw.x, zw.y }; }
Vec4 v4(Vec3 xyz, float w) { return Vec4 { xyz.x, xyz.y, xyz.z, w  }; }
Vec4 v4(float x, Vec3 yzw) { return Vec4 { x, yzw.x, yzw.y, yzw.z }; }


Vec2 v2(Vec3 xy) { return Vec2 { xy.x, xy.y }; }
Vec2 v2(Vec4 xy) { return Vec2 { xy.x, xy.y }; };
Vec3 v3(Vec4 xyz) { return Vec3 { xyz.x, xyz.y, xyz.z }; };



Vec2 Add(Vec2 a, Vec2 b) {
    return Vec2 { a.x + b.x, a.y + b.y };
}

Vec3 Add(Vec3 a, Vec3 b) {
    return Vec3 { a.x + b.x, a.y + b.y, a.z + b.z };
}

Vec4 Add(Vec4 a, Vec4 b) {
    return Vec4 { a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w };
}

Vec2 operator+(Vec2 a, Vec2 b)
{
	return Add(a, b);
}

Vec3 operator+(Vec3 a, Vec3 b)
{
	return Add(a, b);
}

Vec4 operator+(Vec4 a, Vec4 b)
{
	return Add(a, b);
}




Vec2 Sub(Vec2 a, Vec2 b) {
    return Vec2 { a.x - b.x, a.y - b.y };
}

Vec3 Sub(Vec3 a, Vec3 b) {
    return Vec3 { a.x - b.x, a.y - b.y, a.z - b.z };
}

Vec4 Sub(Vec4 a, Vec4 b) {
    return Vec4 { a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w };
}


Vec2 operator-(Vec2 a, Vec2 b)
{
	return Sub(a, b);
}

Vec3 operator-(Vec3 a, Vec3 b)
{
	return Sub(a, b);
}

Vec4 operator-(Vec4 a, Vec4 b)
{
	return Sub(a, b);
}





Vec2 Mul(Vec2 a, Vec2 b) {
    return Vec2 { a.x * b.x, a.y * b.y };
}

Vec3 Mul(Vec3 a, Vec3 b) {
    return Vec3 { a.x * b.x, a.y * b.y, a.z * b.z };
}

Vec4 Mul(Vec4 a, Vec4 b) {
    return Vec4 { a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w };
}



Vec2 operator*(Vec2 a, Vec2 b) 
{
	return Mul(a, b);
}

Vec3 operator*(Vec3 a, Vec3 b) 
{
	return Mul(a, b);
}

Vec4 operator*(Vec4 a, Vec4 b) 
{
	return Mul(a, b);
}



Vec2 Div(Vec2 a, Vec2 b) {
    return Vec2 { a.x / b.x, a.y / b.y };
}

Vec3 Div(Vec3 a, Vec3 b) {
    return Vec3 { a.x / b.x, a.y / b.y, a.z / b.z };
}

Vec4 Div(Vec4 a, Vec4 b) {
    return Vec4 { a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w };
}


Vec2 operator/(Vec2 a, Vec2 b)
{
	return Div(a, b);
}

Vec3 operator/(Vec3 a, Vec3 b)
{
	return Div(a, b);
}

Vec4 operator/(Vec4 a, Vec4 b)
{
	return Div(a, b);
}



float Dot(Vec2 a, Vec2 b) {
    return a.x * b.x + a.y * b.y;
}

float Dot(Vec3 a, Vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

float Dot(Vec4 a, Vec4 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}



Vec3 Cross(Vec3 lhs, Vec3 rhs) { return v3(lhs.y * rhs.z - lhs.z * rhs.y, lhs.z * rhs.x - lhs.x * rhs.z, lhs.x * rhs.y - lhs.y * rhs.x); }


float Length(Vec2 a) {
    return sqrtf(a.x * a.x + a.y * a.y);
}

float Length(Vec3 a) {
    return sqrtf(a.x * a.x + a.y * a.y + a.z * a.z);
}

float Length(Vec4 a) {
    return sqrtf(a.x * a.x + a.y * a.y + a.z * a.z + a.w * a.w);
}

// TODO: optimize lol
Vec2 Normalize(Vec2 a) {
    float len = Length(a);
    if (len == 0) len = 1;
    return Div(a, v2(len));
}

Vec3 Normalize(Vec3 a) {
    float len = Length(a);
    if (len == 0) len = 1;
    return Div(a, v3(len));
}

Vec4 Normalize(Vec4 a) {
    float len = Length(a);
    if (len == 0) len = 1;
    return Div(a, v4(len));
}



bool Equal(Vec2 a, Vec2 b)
{
	return (a.x == b.x) && (a.y == b.y);
}

bool Equal(Vec3 a, Vec3 b)
{
	return (a.x == b.x) && (a.y == b.y) && (a.z == b.z);
}

bool Equal(Vec4 a, Vec4 b)
{
	return (a.x == b.x) && (a.y == b.y) && (a.z == b.z) && (a.w == b.w);
}


// Matrices




Mat4 CreateMatrix(float trace) {
    Mat4 m;
    
    m.m[0 ] = trace;
    m.m[1 ] = 0;
    m.m[2 ] = 0;
    m.m[3 ] = 0;
    
    m.m[4 ] = 0;
    m.m[5 ] = trace;
    m.m[6 ] = 0;
    m.m[7 ] = 0;
    
    m.m[8 ] = 0;
    m.m[9 ] = 0;
    m.m[10] = trace;
    m.m[11] = 0;
    
    m.m[12] = 0;
    m.m[13] = 0;
    m.m[14] = 0;
    m.m[15] = trace;
    
    return m;
}


Mat4 CreateMatrix(float m00, float m10, float m20, float m30,
                  float m01, float m11, float m21, float m31,
                  float m02, float m12, float m22, float m32,
                  float m03, float m13, float m23, float m33) {
    Mat4 mat;
    
    mat.m[0 + 0 * 4] = m00;
	mat.m[1 + 0 * 4] = m10;
	mat.m[2 + 0 * 4] = m20;
	mat.m[3 + 0 * 4] = m30;
    
	mat.m[0 + 1 * 4] = m01;
	mat.m[1 + 1 * 4] = m11;
	mat.m[2 + 1 * 4] = m21;
	mat.m[3 + 1 * 4] = m31;
    
	mat.m[0 + 2 * 4] = m02;
	mat.m[1 + 2 * 4] = m12;
	mat.m[2 + 2 * 4] = m22;
	mat.m[3 + 2 * 4] = m32;
    
	mat.m[0 + 3 * 4] = m03;
	mat.m[1 + 3 * 4] = m13;
	mat.m[2 + 3 * 4] = m23;
	mat.m[3 + 3 * 4] = m33;
    
    return mat;
}



Mat4 Mul(Mat4 a, Mat4 b) {
    Mat4 product;
    
    for (int row = 0; row < 4; row++) {
        Vec4 rowVector = a.rows[row];
        for (int col = 0; col < 4; col++) {
            Vec4 colVector = v4(b.m[col], b.m[col + 4], b.m[col + 8], b.m[col + 12]);
            
            product.m[4 * row + col] = Dot(rowVector, colVector);
        }
    }
    
    return product;
}



Mat4 PerspectiveMatrix(float fov, float aspect, float start, float end) 
{
	float tfov2 = Tan(fov / 2.0);
    return CreateMatrix(1.0 / (aspect * tfov2), 0, 0, 0,
                        0, 1.0 / (tfov2), 0, 0,
                        0, 0, (end) / (end - start), -(end * start) / (end - start),
                        0, 0, 1, 0);
}

Mat4 OrthoMatrix(float left, float right, float bottom, float top, float start, float end)
{
	return CreateMatrix(2 / (right - left), 0, 0, -(right + left) / (right - left),
						0, 2 / (top - bottom), 0, -(top + bottom) / (top - bottom),
						0, 0, 1 / (end - start),  -start / (end - start),
						0, 0, 0, 1
						);
}


Mat4 LookAtMatrix(Vec3 dir, Vec3 up)
{
	Vec3 right = Normalize(Cross(up, dir));
	Vec3 actualUp = Normalize(Cross(dir, right));
	Vec3 dirNorm = Normalize(dir);

	return CreateMatrix(
		right.x, actualUp.x, dirNorm.x, 0,
		right.y, actualUp.y, dirNorm.y, 0,
		right.z, actualUp.z, dirNorm.z, 0,
		0,		 0, 		 0,			1
	);
}

Mat4 TranslationMatrix(Vec3 translation) {
    return CreateMatrix(
                        1, 0, 0, translation.x,
                        0, 1, 0, translation.y,
                        0, 0, 1, translation.z,
                        0, 0, 0, 1);
}

Mat4 RotationMatrixAxisAngle(Vec3 axis, float angle)
{
    axis = Normalize(axis);
    float s = Sin(angle);
    float c = Cos(angle);
    float oc = 1.0 - c;
    
    return CreateMatrix(oc * axis.x * axis.x + c,           oc * axis.x * axis.y - axis.z * s,  oc * axis.z * axis.x + axis.y * s,  0.0,
					    oc * axis.x * axis.y + axis.z * s,  oc * axis.y * axis.y + c,           oc * axis.y * axis.z - axis.x * s,  0.0,
					    oc * axis.z * axis.x - axis.y * s,  oc * axis.y * axis.z + axis.x * s,  oc * axis.z * axis.z + c,           0.0,
					    0.0,                                0.0,                                0.0,                                1.0);
}


Mat4 ScaleMatrix(Vec3 scale) {
    return CreateMatrix(scale.x, 0, 0, 0,
                        0, scale.y, 0, 0,
                        0, 0, scale.z, 0,
                        0, 0, 0, 1);
}






Mat4 Transpose(Mat4 mat) {
	Mat4 result;
	for (u8 col = 0; col < 4; col++) {
		for (u8 row = 0; row < 4; row++) {
			result.m[col + row * 4] = mat.m[row + col * 4];
		}
	}
	return result;
}

Mat4 Inverse(Mat4 mat) {
	// NOTE:
	// we have to transpose the matrix because this function is based off
	// a stackoverflow result, and for some reason, everyone likes to use column major matrices
	// be we use row major, so we have to transpose it, which essentially makes our
	// matrix column major, because i am wayyy to lazy to go change the array indices to make the calculation row major
	// -... 2020-02-08
	Mat4 input = Transpose(mat);

	float* m = input.m;
	float inv[16], det, invOut[16];
	int i;

	inv[0] = m[5] * m[10] * m[15] -
		m[5] * m[11] * m[14] -
		m[9] * m[6] * m[15] +
		m[9] * m[7] * m[14] +
		m[13] * m[6] * m[11] -
		m[13] * m[7] * m[10];

	inv[4] = -m[4] * m[10] * m[15] +
		m[4] * m[11] * m[14] +
		m[8] * m[6] * m[15] -
		m[8] * m[7] * m[14] -
		m[12] * m[6] * m[11] +
		m[12] * m[7] * m[10];

	inv[8] = m[4] * m[9] * m[15] -
		m[4] * m[11] * m[13] -
		m[8] * m[5] * m[15] +
		m[8] * m[7] * m[13] +
		m[12] * m[5] * m[11] -
		m[12] * m[7] * m[9];

	inv[12] = -m[4] * m[9] * m[14] +
		m[4] * m[10] * m[13] +
		m[8] * m[5] * m[14] -
		m[8] * m[6] * m[13] -
		m[12] * m[5] * m[10] +
		m[12] * m[6] * m[9];

	inv[1] = -m[1] * m[10] * m[15] +
		m[1] * m[11] * m[14] +
		m[9] * m[2] * m[15] -
		m[9] * m[3] * m[14] -
		m[13] * m[2] * m[11] +
		m[13] * m[3] * m[10];

	inv[5] = m[0] * m[10] * m[15] -
		m[0] * m[11] * m[14] -
		m[8] * m[2] * m[15] +
		m[8] * m[3] * m[14] +
		m[12] * m[2] * m[11] -
		m[12] * m[3] * m[10];

	inv[9] = -m[0] * m[9] * m[15] +
		m[0] * m[11] * m[13] +
		m[8] * m[1] * m[15] -
		m[8] * m[3] * m[13] -
		m[12] * m[1] * m[11] +
		m[12] * m[3] * m[9];

	inv[13] = m[0] * m[9] * m[14] -
		m[0] * m[10] * m[13] -
		m[8] * m[1] * m[14] +
		m[8] * m[2] * m[13] +
		m[12] * m[1] * m[10] -
		m[12] * m[2] * m[9];

	inv[2] = m[1] * m[6] * m[15] -
		m[1] * m[7] * m[14] -
		m[5] * m[2] * m[15] +
		m[5] * m[3] * m[14] +
		m[13] * m[2] * m[7] -
		m[13] * m[3] * m[6];

	inv[6] = -m[0] * m[6] * m[15] +
		m[0] * m[7] * m[14] +
		m[4] * m[2] * m[15] -
		m[4] * m[3] * m[14] -
		m[12] * m[2] * m[7] +
		m[12] * m[3] * m[6];

	inv[10] = m[0] * m[5] * m[15] -
		m[0] * m[7] * m[13] -
		m[4] * m[1] * m[15] +
		m[4] * m[3] * m[13] +
		m[12] * m[1] * m[7] -
		m[12] * m[3] * m[5];

	inv[14] = -m[0] * m[5] * m[14] +
		m[0] * m[6] * m[13] +
		m[4] * m[1] * m[14] -
		m[4] * m[2] * m[13] -
		m[12] * m[1] * m[6] +
		m[12] * m[2] * m[5];

	inv[3] = -m[1] * m[6] * m[11] +
		m[1] * m[7] * m[10] +
		m[5] * m[2] * m[11] -
		m[5] * m[3] * m[10] -
		m[9] * m[2] * m[7] +
		m[9] * m[3] * m[6];

	inv[7] = m[0] * m[6] * m[11] -
		m[0] * m[7] * m[10] -
		m[4] * m[2] * m[11] +
		m[4] * m[3] * m[10] +
		m[8] * m[2] * m[7] -
		m[8] * m[3] * m[6];

	inv[11] = -m[0] * m[5] * m[11] +
		m[0] * m[7] * m[9] +
		m[4] * m[1] * m[11] -
		m[4] * m[3] * m[9] -
		m[8] * m[1] * m[7] +
		m[8] * m[3] * m[5];

	inv[15] = m[0] * m[5] * m[10] -
		m[0] * m[6] * m[9] -
		m[4] * m[1] * m[10] +
		m[4] * m[2] * m[9] +
		m[8] * m[1] * m[6] -
		m[8] * m[2] * m[5];

	det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];

	// TODO @ErrorHandling, make the error handling better
	Assert(det != 0); // Divide be zero
	det = 1.0 / det;

	for (i = 0; i < 16; i++)
		invOut[i] = inv[i] * det;

	Mat4 result = {};
	Memcpy(result.m, invOut, 16 * sizeof(float));

	// make our matrix back into row major
	return Transpose(result);
}



Vec4 Mul(Vec4 v, Mat4 mat)
{
	// [1, 0, 0, 0] [x]     [x']
	// [0, 1, 0, 0]	[y]     [y']
	// [0, 0, 1, 0] [z]  =  [z']
	// [0, 0, 0, 1] [w]     [w']

	Vec4 res;
	res.x = Dot(mat.rows[0], v);
	res.y = Dot(mat.rows[1], v);
	res.z = Dot(mat.rows[2], v);
	res.w = Dot(mat.rows[3], v);
	return res;
}

Vec3 Mul(Vec3 v, float w, Mat4 mat)
{
	Vec4 vec4 = v4(v, w);
	return v3(Mul(vec4, mat));
}


Vec3 Project(Vec3 v, Mat4 projectionMatrix)
{
	Vec4 proj = Mul(v4(v, 1), projectionMatrix);
	proj = Div(proj, v4(proj.w));
	proj.x = proj.x / 2.0 + 0.5;
	proj.y = proj.y / 2.0 + 0.5;
	return v3(proj);
}

Vec3 Unproject(Vec3 ndc, Mat4 projectionMatrix)
{
	ndc.x = ndc.x * 2 - 1;
	ndc.y = ndc.y * 2 - 1;
	Vec4 mv = Mul(v4(ndc, 1), Inverse(projectionMatrix));
	mv = Div(mv, v4(mv.w));
	return v3(mv);
}



Xform CreateXform() {
	return Xform {
		v3(0),
		CreateMatrix(1),
		v3(1)
	};
}

Mat4 XformMatrix(Xform xform) {
	return 	Mul(
				TranslationMatrix(xform.position),
				Mul(
					xform.rotation,
					ScaleMatrix(xform.scale)
					)
				);
}




Vec3 OrientForward(Mat4 mat) {
	Vec3 col = v3(
		mat.m[2 + 4 * 0],
		mat.m[2 + 4 * 1],
		mat.m[2 + 4 * 2]
	);
	return col;
}

Vec3 OrientUp(Mat4 mat) {
	Vec3 col = v3(
		mat.m[1 + 4 * 0],
		mat.m[1 + 4 * 1],
		mat.m[1 + 4 * 2]
	);
	return col;
}

Vec3 OrientRight(Mat4 mat) {
	Vec3 col = v3(
		mat.m[0 + 4 * 0],
		mat.m[0 + 4 * 1],
		mat.m[0 + 4 * 2]
	);
	return col;
}

///


float Sin(float x) {
	return sinf(x / 180.0 * PI);
}

float Cos(float x) {
	return cosf(x / 180.0 * PI);
}

float Tan(float x) {
	return tanf(x / 180.0 * PI);
}

float Atan(float x)
{
	return atanf(x) / PI * 180;
}

float Acos(float x)
{
	return acosf(x) / PI * 180;
}


float Abs(float x)
{
	if (x < 0)
		return -x;
	return x;
}


float Sqrt(float x)
{
	return sqrtf(x);
}


float Min(float x, float y) {
	return (x < y) ? x : y;
}

float Max(float x, float y) {
	return (x > y) ? x : y;
}

float Clamp(float v, float min, float max) {
	return Max(Min(v, max), min);
}


float Floor(float x) {
	return floorf(x);
}

float Fract(float x) {
	return x - Floor(x);
}

float Lerp(float a, float b, float t)
{
	return a * (1 - t) + b * t;
}



s32 PowS32(s32 base, s32 exponent)
{
	if (exponent < 0)
		return 0;

	s32 x = 1;
	for (u64 i = 0; i < exponent; i++)
	{
		x *= base;
	}
	return x;
}

u64 PowU64(u64 base, u64 exponent)
{
	u64 x = 1;
	for (u64 i = 0; i < exponent; i++)
	{
		x *= base;
	}
	return x;
}

float PowF(float base, float exponent)
{
	return powf(base, exponent);
}