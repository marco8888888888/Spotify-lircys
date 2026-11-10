#include "orbitcore/Patched.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace oc {

const char* ToString(SegmentEnd e) {
  switch (e) {
    case SegmentEnd::Horizon: return "Horizon";
    case SegmentEnd::SoiExit: return "SoiExit";
    case SegmentEnd::SoiEntry: return "SoiEntry";
    case SegmentEnd::Impact: return "Impact";
  }
  return "?";
}

StateVector PatchedSegment::StateAtTime(double t) const {
  StateVector s = StateAt(elements, mu, t);
  s.bodyId = bodyId;
  return s;
}

std::optional<double> FindSoiEntry(const OrbitalElements& el, double mu, const System& system,
                                   const std::string& childId, double tStart, double tStop,
                                   const PatchedOptions& options) {
  const CelestialBody& child = system.Get(childId);
  if (!child.orbit || !(child.soiRadius > 0.0) || !(tStop > tStart)) return std::nullopt;
  const double soi = child.soiRadius;

  // Quick reject: radial ranges of craft and child SOI do not overlap.
  const double cPe = Periapsis(*child.orbit) - soi;
  const double cAp = Apoapsis(*child.orbit) + soi;
  if (Apoapsis(el) < cPe || Periapsis(el) > cAp) return std::nullopt;

  auto rel = [&](double t, double* relSpeed) {
    const StateVector c = system.BodyStateRelativeToParent(childId, t);
    const StateVector v = StateAt(el, mu, t);
    if (relSpeed) *relSpeed = Length(v.v - c.v);
    return Length(v.r - c.r);
  };

  double maxStep = Period(*child.orbit, system.Get(child.parentId).mu);
  maxStep = std::min(maxStep, Period(el, mu));
  maxStep *= options.maxStepPeriodFraction;
  if (!std::isfinite(maxStep) || maxStep <= 0.0) maxStep = tStop - tStart;

  double t = tStart;
  double vrel = 0.0;
  double d = rel(t, &vrel);
  bool armed = d > soi;
  for (int it = 0; it < options.maxSearchSteps && t < tStop; ++it) {
    const double vr = std::max(vrel, 1e-6);
    const double minStep = std::min(options.minStepSoiFraction * soi / vr, maxStep);
    double step = 0.5 * std::fabs(d - soi) / vr;
    step = std::clamp(step, minStep, maxStep);
    const double tn = std::min(t + step, tStop);
    double vn = 0.0;
    const double dn = rel(tn, &vn);
    if (armed && dn <= soi) {
      double lo = t, hi = tn;
      for (int k = 0; k < 100 && hi - lo > 1e-9 * std::max(1.0, std::fabs(hi)); ++k) {
        const double mid = 0.5 * (lo + hi);
        if (rel(mid, nullptr) <= soi) hi = mid; else lo = mid;
      }
      return hi;  // first point inside the SOI
    }
    if (!armed && dn > soi) armed = true;
    t = tn;
    d = dn;
    vrel = vn;
  }
  return std::nullopt;
}

std::vector<PatchedSegment> PredictPatched(const StateVector& s, const System& system, double horizon,
                                           const PatchedOptions& options) {
  std::vector<PatchedSegment> out;
  if (!system.Find(s.bodyId)) return out;
  const double tEndAll = s.t + std::max(0.0, std::isfinite(horizon) ? horizon : 0.0);
  StateVector cur = system.ToDominantBody(s);

  for (int segIdx = 0; segIdx < options.maxSegments; ++segIdx) {
    const CelestialBody& body = system.Get(cur.bodyId);
    PatchedSegment seg;
    seg.bodyId = body.id;
    seg.mu = body.mu;
    seg.elements = ElementsFromState(cur, body.mu);
    seg.startTime = cur.t;
    seg.startState = cur;

    double tLimit = tEndAll;
    SegmentEnd reason = SegmentEnd::Horizon;
    std::string next;

    // 1) SOI exit to the parent.
    if (!body.parentId.empty() && body.soiRadius > 0.0) {
      if (Length(cur.r) >= body.soiRadius && Dot(cur.r, cur.v) > 0.0) {
        tLimit = cur.t;
        reason = SegmentEnd::SoiExit;
        next = body.parentId;
      } else if (auto te = NextTimeAtRadius(seg.elements, body.mu, body.soiRadius, cur.t, true)) {
        if (*te < tLimit) {
          tLimit = *te;
          reason = SegmentEnd::SoiExit;
          next = body.parentId;
        }
      }
    }

    // 2) Surface impact.
    if (options.stopAtImpact && body.radius > 0.0 && Periapsis(seg.elements) < body.radius) {
      std::optional<double> ti;
      if (Length(cur.r) <= body.radius && Dot(cur.r, cur.v) < 0.0) {
        ti = cur.t;
      } else {
        ti = NextTimeAtRadius(seg.elements, body.mu, body.radius, cur.t, false);
      }
      if (ti && *ti < tLimit) {
        tLimit = *ti;
        reason = SegmentEnd::Impact;
        next.clear();
      }
    }

    // 3) SOI entry into a child body.
    for (const std::string& cid : system.ChildrenOf(body.id)) {
      if (auto tc = FindSoiEntry(seg.elements, body.mu, system, cid, cur.t, tLimit, options)) {
        if (*tc < tLimit) {
          tLimit = *tc;
          reason = SegmentEnd::SoiEntry;
          next = cid;
        }
      }
    }

    seg.endTime = tLimit;
    seg.endState = seg.StateAtTime(tLimit);
    seg.endReason = reason;
    seg.nextBodyId = next;
    out.push_back(seg);

    if (reason == SegmentEnd::Horizon || reason == SegmentEnd::Impact || tLimit >= tEndAll) break;
    cur = system.Transform(seg.endState, next);
  }
  return out;
}

std::vector<PatchedSegment> PredictPatched(const StateVector& s, const System& system, double horizon) {
  return PredictPatched(s, system, horizon, PatchedOptions{});
}

}  // namespace oc
