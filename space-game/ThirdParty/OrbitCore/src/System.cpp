#include "orbitcore/System.h"

#include <cmath>
#include <stdexcept>
#include <unordered_set>

namespace oc {

double ComputeSoiRadius(double semiMajorAxis, double muChild, double muParent) {
  if (!(muParent > 0.0) || !(muChild > 0.0)) return 0.0;
  return std::fabs(semiMajorAxis) * std::pow(muChild / muParent, 0.4);
}

System::System(std::vector<CelestialBody> bodies) : bodies_(std::move(bodies)) { RebuildIndex(); }

void System::AddBody(CelestialBody body) {
  auto it = index_.find(body.id);
  if (it != index_.end()) {
    bodies_[it->second] = std::move(body);
  } else {
    bodies_.push_back(std::move(body));
  }
  RebuildIndex();
}

void System::RebuildIndex() {
  index_.clear();
  children_.clear();
  for (size_t k = 0; k < bodies_.size(); ++k) index_[bodies_[k].id] = k;
  for (const auto& b : bodies_) {
    if (!b.parentId.empty()) children_[b.parentId].push_back(b.id);
  }
  for (auto& b : bodies_) {
    if (b.soiRadius > 0.0 || b.parentId.empty() || !b.orbit) continue;
    const CelestialBody* parent = Find(b.parentId);
    if (parent) b.soiRadius = ComputeSoiRadius(b.orbit->a, b.mu, parent->mu);
  }
}

bool System::Validate(std::string* error) const {
  auto fail = [&](const std::string& msg) {
    if (error) *error = msg;
    return false;
  };
  int roots = 0;
  for (const auto& b : bodies_) {
    if (!(b.mu > 0.0)) return fail("body '" + b.id + "' has mu <= 0");
    if (b.parentId.empty()) {
      ++roots;
      continue;
    }
    if (!Find(b.parentId)) return fail("body '" + b.id + "' has unknown parent '" + b.parentId + "'");
    if (!b.orbit) return fail("body '" + b.id + "' has a parent but no orbit");
    if (!IsFinite(*b.orbit)) return fail("body '" + b.id + "' has non-finite orbit");
    if (b.orbit->e >= 1.0) return fail("body '" + b.id + "' must have a closed orbit (e < 1)");
    std::unordered_set<std::string> seen;
    for (const CelestialBody* c = &b; c && !c->parentId.empty(); c = Find(c->parentId)) {
      if (!seen.insert(c->id).second) return fail("cycle in hierarchy at '" + b.id + "'");
    }
  }
  if (roots != 1) return fail("exactly one root body (empty parentId) required");
  return true;
}

const CelestialBody* System::Find(const std::string& id) const {
  auto it = index_.find(id);
  return it == index_.end() ? nullptr : &bodies_[it->second];
}

const CelestialBody& System::Get(const std::string& id) const {
  const CelestialBody* b = Find(id);
  if (!b) throw std::out_of_range("oc::System: unknown body '" + id + "'");
  return *b;
}

std::string System::RootId() const {
  for (const auto& b : bodies_)
    if (b.parentId.empty()) return b.id;
  return {};
}

const std::vector<std::string>& System::ChildrenOf(const std::string& id) const {
  static const std::vector<std::string> kEmpty;
  auto it = children_.find(id);
  return it == children_.end() ? kEmpty : it->second;
}

int System::Depth(const std::string& id) const {
  int d = 0;
  for (const CelestialBody* b = Find(id); b && !b->parentId.empty() && d < 64; b = Find(b->parentId)) ++d;
  return d;
}

StateVector System::BodyStateRelativeToParent(const std::string& id, double t) const {
  const CelestialBody& b = Get(id);
  if (b.parentId.empty() || !b.orbit) return StateVector{{0, 0, 0}, {0, 0, 0}, t, b.parentId};
  StateVector s = StateAt(*b.orbit, Get(b.parentId).mu, t);
  s.bodyId = b.parentId;
  return s;
}

StateVector System::BodyStateAbsolute(const std::string& id, double t) const {
  StateVector acc{{0, 0, 0}, {0, 0, 0}, t, RootId()};
  int guard = 0;
  for (const CelestialBody* b = &Get(id); b && !b->parentId.empty() && guard < 64; b = Find(b->parentId), ++guard) {
    const StateVector rel = BodyStateRelativeToParent(b->id, t);
    acc.r += rel.r;
    acc.v += rel.v;
  }
  return acc;
}

StateVector System::Transform(const StateVector& s, const std::string& targetBodyId) const {
  if (s.bodyId == targetBodyId) return s;
  const StateVector from = BodyStateAbsolute(s.bodyId, s.t);
  const StateVector to = BodyStateAbsolute(targetBodyId, s.t);
  StateVector out;
  out.r = s.r + from.r - to.r;
  out.v = s.v + from.v - to.v;
  out.t = s.t;
  out.bodyId = targetBodyId;
  return out;
}

std::string System::FindDominantBody(const Vec3d& rAbs, double t) const {
  std::string current = RootId();
  Vec3d currentAbs{0, 0, 0};
  for (int depth = 0; depth < 64; ++depth) {
    std::string best;
    Vec3d bestAbs{};
    double bestRatio = 1.0;
    for (const std::string& cid : ChildrenOf(current)) {
      const CelestialBody& c = Get(cid);
      if (!(c.soiRadius > 0.0)) continue;
      const Vec3d cAbs = currentAbs + BodyStateRelativeToParent(cid, t).r;
      const double ratio = Length(rAbs - cAbs) / c.soiRadius;
      if (ratio < bestRatio) {
        bestRatio = ratio;
        best = cid;
        bestAbs = cAbs;
      }
    }
    if (best.empty()) break;
    current = best;
    currentAbs = bestAbs;
  }
  return current;
}

StateVector System::ToDominantBody(const StateVector& s) const {
  const StateVector abs = Transform(s, RootId());
  return Transform(abs, FindDominantBody(abs.r, s.t));
}

bool System::IsOutsideSoi(const StateVector& s) const {
  const CelestialBody& b = Get(s.bodyId);
  if (b.parentId.empty() || !(b.soiRadius > 0.0)) return false;
  return Length(s.r) > b.soiRadius;
}

Vec3d System::GravityAcceleration(const StateVector& s) const {
  return oc::GravityAcceleration(s.r, Get(s.bodyId).mu);
}

}  // namespace oc
