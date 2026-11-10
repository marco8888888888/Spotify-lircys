// Body hierarchy (star -> planets -> moons), body ephemerides on rails, SOI handling.
// Addition to PROMPT.md section 5 (forward-declared there as `class System`).
#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "orbitcore/Orbit.h"
#include "orbitcore/Types.h"

namespace oc {

// Laplace sphere-of-influence radius: a * (muChild / muParent)^(2/5).
double ComputeSoiRadius(double semiMajorAxis, double muChild, double muParent);

class System {
 public:
  System() = default;
  // Adds all bodies (any order) and fills missing soiRadius values (<= 0) via ComputeSoiRadius.
  explicit System(std::vector<CelestialBody> bodies);

  // Adds/replaces a body. Missing soiRadius is computed once the parent is known.
  void AddBody(CelestialBody body);

  // Checks the hierarchy: exactly one root (empty parentId), parents exist, no cycles,
  // non-root bodies have an orbit, mu > 0. Returns false and fills `error` on failure.
  bool Validate(std::string* error = nullptr) const;

  const CelestialBody* Find(const std::string& id) const;
  const CelestialBody& Get(const std::string& id) const;  // throws std::out_of_range
  const std::vector<CelestialBody>& Bodies() const { return bodies_; }
  std::string RootId() const;
  const std::vector<std::string>& ChildrenOf(const std::string& id) const;
  // Depth in the hierarchy (root = 0).
  int Depth(const std::string& id) const;

  // State of body `id` relative to its parent (bodyId = parentId). Root: zero vectors.
  StateVector BodyStateRelativeToParent(const std::string& id, double t) const;
  // State of body `id` relative to the root body (bodyId = root id).
  StateVector BodyStateAbsolute(const std::string& id, double t) const;

  // Re-expresses state `s` (relative to s.bodyId at s.t) relative to `targetBodyId`.
  StateVector Transform(const StateVector& s, const std::string& targetBodyId) const;

  // Deepest body whose SOI contains the root-relative position rAbs at time t.
  std::string FindDominantBody(const Vec3d& rAbs, double t) const;
  // Re-expresses `s` relative to the body whose SOI it is actually in.
  StateVector ToDominantBody(const StateVector& s) const;
  // True if |s.r| exceeds the SOI of s.bodyId (root never).
  bool IsOutsideSoi(const StateVector& s) const;

  // Patched-conic gravity: point-mass acceleration from s.bodyId only (for physics on rails / Chaos).
  Vec3d GravityAcceleration(const StateVector& s) const;

 private:
  void RebuildIndex();
  std::vector<CelestialBody> bodies_;
  std::unordered_map<std::string, size_t> index_;
  std::unordered_map<std::string, std::vector<std::string>> children_;
};

}  // namespace oc
