#pragma once

#include <cmath>

#include "doctest.h"
#include "orbitcore/OrbitCore.h"

namespace octest {

// Fictional test bodies (round numbers, not taken from any game).
constexpr double kMuStar = 1.3e20;
constexpr double kMuPlanet = 4.0e14;
constexpr double kRadiusPlanet = 6.4e6;
constexpr double kMuMoon = 5.0e12;
constexpr double kRadiusMoon = 1.7e6;
constexpr double kMoonA = 3.8e8;
constexpr double kPlanetA = 1.5e11;

inline bool Finite(const oc::StateVector& s) { return oc::IsFinite(s.r) && oc::IsFinite(s.v) && std::isfinite(s.t); }

inline double RelErr(const oc::Vec3d& a, const oc::Vec3d& b) {
  const double n = std::max(oc::Length(b), 1e-300);
  return oc::Length(a - b) / n;
}

inline oc::System MakeTestSystem() {
  std::vector<oc::CelestialBody> bodies;
  oc::CelestialBody star{"sun", "Testsonne", "", kMuStar, 7e8, 0.0, 2e6, std::nullopt, std::nullopt};
  oc::CelestialBody planet{"planet", "Testplanet", "sun", kMuPlanet, kRadiusPlanet, 0.0, 86000.0,
                           oc::OrbitalElements{kPlanetA, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
                           oc::Atmosphere{70000.0, 101325.0, 5600.0}};
  oc::CelestialBody moon{"moon", "Testmond", "planet", kMuMoon, kRadiusMoon, 0.0, 2.3e6,
                         oc::OrbitalElements{kMoonA, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0}, std::nullopt};
  // deliberately out of order
  bodies.push_back(moon);
  bodies.push_back(star);
  bodies.push_back(planet);
  return oc::System(bodies);
}

}  // namespace octest
