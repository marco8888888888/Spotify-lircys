// Small inline vector math for oc::Vec3d.
#pragma once

#include <cmath>

#include "orbitcore/Types.h"

namespace oc {

inline Vec3d operator+(const Vec3d& a, const Vec3d& b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
inline Vec3d operator-(const Vec3d& a, const Vec3d& b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
inline Vec3d operator-(const Vec3d& a) { return {-a.x, -a.y, -a.z}; }
inline Vec3d operator*(const Vec3d& a, double s) { return {a.x * s, a.y * s, a.z * s}; }
inline Vec3d operator*(double s, const Vec3d& a) { return a * s; }
inline Vec3d operator/(const Vec3d& a, double s) { return {a.x / s, a.y / s, a.z / s}; }
inline Vec3d& operator+=(Vec3d& a, const Vec3d& b) { a = a + b; return a; }
inline Vec3d& operator-=(Vec3d& a, const Vec3d& b) { a = a - b; return a; }

inline double Dot(const Vec3d& a, const Vec3d& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline Vec3d Cross(const Vec3d& a, const Vec3d& b) {
  return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
inline double Length(const Vec3d& a) { return std::sqrt(Dot(a, a)); }
inline double LengthSq(const Vec3d& a) { return Dot(a, a); }
inline Vec3d Normalize(const Vec3d& a) {
  const double l = Length(a);
  return l > 0.0 ? a / l : Vec3d{0.0, 0.0, 0.0};
}
inline bool IsFinite(const Vec3d& a) { return std::isfinite(a.x) && std::isfinite(a.y) && std::isfinite(a.z); }

}  // namespace oc
