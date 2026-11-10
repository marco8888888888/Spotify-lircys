// OrbitCore - engine-free orbital mechanics for ORBITWERK.
// Types.h: data contracts from PROMPT.md section 5 (do not change without the orchestrator).
// namespace oc, all values double, SI units (m, s, kg, rad).
#pragma once

#include <optional>
#include <string>
#include <vector>

namespace oc {

struct Vec3d { double x, y, z; };

// Classical Keplerian elements.
//  a      semi-major axis [m]; a > 0 ellipse, a < 0 hyperbola
//  e      eccentricity [-]
//  i      inclination [rad] in [0, pi]
//  lan    longitude of ascending node [rad] in [0, 2pi)
//  argPe  argument of periapsis [rad] in [0, 2pi)
//  meanAnomalyAtEpoch [rad] at epoch; ellipse in [-pi, pi), hyperbola: e*sinh(H)-H (unbounded)
//  epoch  [s]
struct OrbitalElements { double a, e, i, lan, argPe, meanAnomalyAtEpoch, epoch; };

struct Atmosphere { double height, seaLevelPressure, scaleHeight; };

struct CelestialBody {
  std::string id, name, parentId;   // parentId leer = Stern
  double mu, radius, soiRadius, rotationPeriod;
  std::optional<OrbitalElements> orbit;
  std::optional<Atmosphere> atmosphere;
};

// Position/velocity relative to the centre of body `bodyId` (non-rotating frame), at time t.
struct StateVector { Vec3d r, v; double t; std::string bodyId; };

struct PatchedSegment;  // defined in orbitcore/Patched.h
class System;           // defined in orbitcore/System.h

// Kern-API (implemented in Orbit.cpp / Patched.cpp)
OrbitalElements ElementsFromState(const StateVector&, double mu);
StateVector     StateAt(const OrbitalElements&, double mu, double t);
std::vector<struct PatchedSegment> PredictPatched(const StateVector&, const class System&, double horizon);

}  // namespace oc
