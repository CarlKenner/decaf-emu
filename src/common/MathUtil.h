// Copyright 2008 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#pragma once

#include <algorithm>
#include <cstdlib>
#include <vector>

inline float SignOf(float x)
{
  // VR 1 if x is positive, -1 if x is negative, or 0 if x is zero
  return (float)((x > 0) - (x < 0));
}

namespace MathUtil
{
#if defined(_MSC_VER) && _MSC_VER <= 1800
template <typename T>
inline T SNANConstant()
{
  return std::numeric_limits<T>::signaling_NaN();
}
#else
template <typename T>
constexpr T SNANConstant()
{
  return std::numeric_limits<T>::signaling_NaN();
}
#endif

#ifdef _MSC_VER

// MSVC needs a workaround, because its std::numeric_limits<double>::signaling_NaN()
// will use __builtin_nans, which is improperly handled by the compiler and generates
// a bad constant. Here we go back to the version MSVC used before the builtin.
// TODO: Remove this and use numeric_limits directly whenever this bug is fixed.
#if _MSC_VER <= 1800
template <>
inline double SNANConstant()
{
  return (_CSTD _Snan._Double);
}
template <>
inline float SNANConstant()
{
  return (_CSTD _Snan._Float);
}
#else
template <>
constexpr double SNANConstant()
{
  return (_CSTD _Snan._Double);
}
template <>
constexpr float SNANConstant()
{
  return (_CSTD _Snan._Float);
}
#endif
#endif

#if defined(_MSC_VER) && _MSC_VER <= 1800
template <class T>
inline T Clamp(const T val, const T& min, const T& max)
#else
template <class T>
constexpr T Clamp(const T val, const T& min, const T& max)
#endif
{
  return std::max(min, std::min(max, val));
}

#if defined(_MSC_VER) && _MSC_VER <= 1800
inline bool IsPow2(uint32_t imm)
#else
constexpr bool IsPow2(uint32_t imm)
#endif
{
  return (imm & (imm - 1)) == 0;
}

// The most significant bit of the fraction is an is-quiet bit on all architectures we care about.

static const uint64_t DOUBLE_SIGN = 0x8000000000000000ULL, DOUBLE_EXP = 0x7FF0000000000000ULL,
                 DOUBLE_FRAC = 0x000FFFFFFFFFFFFFULL, DOUBLE_ZERO = 0x0000000000000000ULL,
                 DOUBLE_QBIT = 0x0008000000000000ULL;

static const uint32_t FLOAT_SIGN = 0x80000000, FLOAT_EXP = 0x7F800000, FLOAT_FRAC = 0x007FFFFF,
                 FLOAT_ZERO = 0x00000000;

union IntDouble
{
  double d;
  uint64_t i;

  explicit IntDouble(uint64_t _i) : i(_i) {}
  explicit IntDouble(double _d) : d(_d) {}
};
union IntFloat
{
  float f;
  uint32_t i;

  explicit IntFloat(uint32_t _i) : i(_i) {}
  explicit IntFloat(float _f) : f(_f) {}
};

inline bool IsQNAN(double d)
{
  IntDouble x(d);
  return ((x.i & DOUBLE_EXP) == DOUBLE_EXP) && ((x.i & DOUBLE_QBIT) == DOUBLE_QBIT);
}

inline bool IsSNAN(double d)
{
  IntDouble x(d);
  return ((x.i & DOUBLE_EXP) == DOUBLE_EXP) && ((x.i & DOUBLE_FRAC) != DOUBLE_ZERO) &&
         ((x.i & DOUBLE_QBIT) == DOUBLE_ZERO);
}

inline float FlushToZero(float f)
{
  IntFloat x(f);
  if ((x.i & FLOAT_EXP) == 0)
  {
    x.i &= FLOAT_SIGN;  // turn into signed zero
  }
  return x.f;
}

inline double FlushToZero(double d)
{
  IntDouble x(d);
  if ((x.i & DOUBLE_EXP) == 0)
  {
    x.i &= DOUBLE_SIGN;  // turn into signed zero
  }
  return x.d;
}

enum PPCFpClass
{
  PPC_FPCLASS_QNAN = 0x11,
  PPC_FPCLASS_NINF = 0x9,
  PPC_FPCLASS_NN = 0x8,
  PPC_FPCLASS_ND = 0x18,
  PPC_FPCLASS_NZ = 0x12,
  PPC_FPCLASS_PZ = 0x2,
  PPC_FPCLASS_PD = 0x14,
  PPC_FPCLASS_PN = 0x4,
  PPC_FPCLASS_PINF = 0x5,
};

// Uses PowerPC conventions for the return value, so it can be easily
// used directly in CPU emulation.
uint32_t ClassifyDouble(double dvalue);
// More efficient float version.
uint32_t ClassifyFloat(float fvalue);

extern const int frsqrte_expected_base[];
extern const int frsqrte_expected_dec[];
extern const int fres_expected_base[];
extern const int fres_expected_dec[];

// PowerPC approximation algorithms
double ApproximateReciprocalSquareRoot(double val);
double ApproximateReciprocal(double val);

template <class T>
struct Rectangle
{
  T left{};
  T top{};
  T right{};
  T bottom{};

#if defined(_MSC_VER) && _MSC_VER <= 1800
  inline Rectangle() = default;

  inline Rectangle(T theLeft, T theTop, T theRight, T theBottom)
      : left(theLeft), top(theTop), right(theRight), bottom(theBottom)
  {
  }

  inline bool operator==(const Rectangle& r) const
#else
  constexpr Rectangle() = default;

  constexpr Rectangle(T theLeft, T theTop, T theRight, T theBottom)
      : left(theLeft), top(theTop), right(theRight), bottom(theBottom)
  {
  }

  constexpr bool operator==(const Rectangle& r) const
#endif
  {
    return left == r.left && top == r.top && right == r.right && bottom == r.bottom;
  }

  T GetWidth() const { return abs(right - left); }
  T GetHeight() const { return abs(bottom - top); }
  // If the rectangle is in a coordinate system with a lower-left origin, use
  // this Clamp.
  void ClampLL(T x1, T y1, T x2, T y2)
  {
    left = Clamp(left, x1, x2);
    right = Clamp(right, x1, x2);
    top = Clamp(top, y2, y1);
    bottom = Clamp(bottom, y2, y1);
  }

  // If the rectangle is in a coordinate system with an upper-left origin,
  // use this Clamp.
  void ClampUL(T x1, T y1, T x2, T y2)
  {
    left = Clamp(left, x1, x2);
    right = Clamp(right, x1, x2);
    top = Clamp(top, y1, y2);
    bottom = Clamp(bottom, y1, y2);
  }
};

}  // namespace MathUtil

float MathFloatVectorSum(const std::vector<float>&);

// Rounds down. 0 -> undefined
inline int IntLog2(uint64_t val)
{
#if defined(__GNUC__)
  return 63 - __builtin_clzll(val);

#elif defined(_MSC_VER)
  unsigned long result = -1;
  _BitScanReverse64(&result, val);
  return result;

#else
  int result = -1;
  while (val != 0)
  {
    val >>= 1;
    ++result;
  }
  return result;
#endif
}

// Tiny matrix/vector library.
// Used for things like Free-Look in the gfx backend.

class Quaternion
{
public:
  static void LoadIdentity(Quaternion& quat);
  static void Set(Quaternion& quat, const float quatArray[4]);

  static void Invert(Quaternion& quat);

  static void Multiply(const Quaternion& a, const Quaternion& b, Quaternion& result);

  // w, x, y, z
  float data[4];
};

class Matrix33
{
public:
  static void LoadIdentity(Matrix33& mtx);
  void setIdentity() { Matrix33::LoadIdentity(*this); }
  void empty() { memset(this->data, 0, sizeof(this->data)); }
  static void LoadQuaternion(Matrix33& mtx, const Quaternion& quat);

  // set mtx to be a rotation matrix around the x axis
  static void RotateX(Matrix33& mtx, float rad);
  // set mtx to be a rotation matrix around the y axis
  static void RotateY(Matrix33& mtx, float rad);
  // set mtx to be a rotation matrix around the z axis
  static void RotateZ(Matrix33& mtx, float rad);

  void setScaling(const float f)
  {
    empty();
    xx = yy = zz = f;
  }
  void setScaling(const float vec[3])
  {
    empty();
    xx = vec[0];
    yy = vec[1];
    zz = vec[2];
  }

  // set result = a x b
  static void Multiply(const Matrix33& a, const Matrix33& b, Matrix33& result);
  static void Multiply(const Matrix33& a, const float vec[3], float result[3]);
  Matrix33 operator*(const Matrix33& rhs) const;
  void operator*=(const Matrix33& rhs) { *this = *this * rhs; }
  static void GetPieYawPitchRollR(const Matrix33& m, float& yaw, float& pitch, float& roll);

  const float& operator[](int i) const { return *(((const float*)this->data) + i); }
  float& operator[](int i) { return *(((float*)this->data) + i); }
  const float* getReadPtr() const { return (const float*)data; }
  void setRight(const float v[3])
  {
    xx = v[0];
    xy = v[1];
    xz = v[2];
  }
  void setUp(const float v[3])
  {
    yx = v[0];
    yy = v[1];
    yz = v[2];
  }
  void setFront(const float v[3])
  {
    zx = v[0];
    zy = v[1];
    zz = v[2];
  }

  union {
    struct
    {
      float xx, yx, zx;
      float xy, yy, zy;
      float xz, yz, zz;
    };
    float data[9];
  };
};

class Matrix44
{
public:
  static void LoadIdentity(Matrix44& mtx);
  void setIdentity() { Matrix44::LoadIdentity(*this); }
  void empty() { memset(this->data, 0, sizeof(this->data)); }
  static void LoadMatrix33(Matrix44& mtx, const Matrix33& m33);
  Matrix44& operator=(const Matrix33& rhs);

  static void Set(Matrix44& mtx, const float mtxArray[16]);
  static void Set3x4(Matrix44& mtx, const float mtxArray[12]);

  static void Translate(Matrix44& mtx, const float vec[3]);
  void setTranslation(const float trans[3]) { Matrix44::Translate(*this, trans); }
  static void Shear(Matrix44& mtx, const float a, const float b = 0);

  static void Scale(Matrix44& mtx, const float vec[3]);
  void setScaling(const float f)
  {
    empty();
    xx = yy = zz = f;
    ww = 1.0f;
  }
  void setScaling(const float vec[3]) { Matrix44::Scale(*this, vec); }
  // Matrix44::Multiply(RHS, LHS, Result); is the same as Result = LHS * RHS;
  static void Multiply(const Matrix44& a, const Matrix44& b, Matrix44& result);
  static void Multiply(const Matrix44& a, const float vec[3], float result[3]);
  Matrix44 operator*(const Matrix44& rhs) const;
  void operator*=(const Matrix44& rhs) { *this = *this * rhs; }
  static void InvertTranslation(Matrix44& mtx);
  static void InvertRotation(Matrix44& mtx);
  static void InvertScale(Matrix44& mtx);

  const float& operator[](int i) const { return *(((const float*)this->data) + i); }
  float& operator[](int i) { return *(((float*)this->data) + i); }
  const float* getReadPtr() const { return (const float*)data; }
  void setTranslationAndScaling(const float trans[3], const float scale[3])
  {
    setScaling(scale);
    wx = trans[0];
    wy = trans[1];
    wz = trans[2];
  }
  void setRight(const float v[3])
  {
    xx = v[0];
    xy = v[1];
    xz = v[2];
  }
  void setUp(const float v[3])
  {
    yx = v[0];
    yy = v[1];
    yz = v[2];
  }
  void setFront(const float v[3])
  {
    zx = v[0];
    zy = v[1];
    zz = v[2];
  }
  void setMove(const float v[3])
  {
    wx = v[0];
    wy = v[1];
    wz = v[2];
  }

  Matrix44 inverse() const;
  Matrix44 simpleInverse() const; // invert a matrix with only rotations and translations
  Matrix44 transpose() const;
  Matrix44 extractOpenGLProjection() const; // extract OpenGL proj matrix from an OpenGL viewProj matrix
  Matrix44 extractOpenGLView() const; // extract view matrix from an OpenGL viewProj matrix
  float simpleHFOV() const; // Horizontal FOV in degrees from straight proj matrix
  float hFOV() const; // Horizontal FOV in degrees from viewProj or proj matrix
  float simpleVFOV() const; // Vertical FOV in degrees from straight proj matrix
  float vFOV() const; // Vertical FOV in degrees from viewProj or proj matrix
  float simpleAspectRatio() const; // rectilinear width/height from straight proj matrix
  float aspectRatio() const; // rectilinear width/height from viewProj or proj matrix
  float openglFar() const;
  float openglNear() const;

  void setRotationX(const float a)
  {
    empty();
    float c = cosf(a);
    float s = sinf(a);
    xx = 1.0f;
    yy = c;
    yz = s;
    zy = -s;
    zz = c;
    ww = 1.0f;
  }
  void setRotationY(const float a)
  {
    empty();
    float c = cosf(a);
    float s = sinf(a);
    xx = c;
    xz = -s;
    yy = 1.0f;
    zx = s;
    zz = c;
    ww = 1.0f;
  }
  void setRotationZ(const float a)
  {
    empty();
    float c = cosf(a);
    float s = sinf(a);
    xx = c;
    xy = s;
    yx = -s;
    yy = c;
    zz = 1.0f;
    ww = 1.0f;
  }

  union {
    // Dolphin stores matrix elements in a different order than PPSSPP
    struct
    {
      float xx, yx, zx, wx;
      float xy, yy, zy, wy;
      float xz, yz, zz, wz;
      float xw, yw, zw, ww;
    };
    float data[16];
  };
};
