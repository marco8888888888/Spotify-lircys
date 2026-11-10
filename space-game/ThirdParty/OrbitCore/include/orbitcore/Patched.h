// Patched-conic trajectory prediction across spheres of influence.
// PatchedSegment is an addition to PROMPT.md section 5 (forward-declared there).
#pragma once

#include <string>
#include <vector>

#include "orbitcore/System.h"
#include "orbitcore/Types.h"

namespace oc {

enum class SegmentEnd {
  Horizon,   // prediction horizon reached
  SoiExit,   // left the SOI of bodyId, continues around nextBodyId (parent)
  SoiEntry,  // entered the SOI of child nextBodyId
  Impact,    // hit the surface (|r| == body.radius, inbound)
};

struct PatchedSegment {
  std::string bodyId;          // reference body of this conic
  double mu = 0.0;             // gravitational parameter of bodyId
  OrbitalElements elements{};  // conic relative to bodyId (epoch = startTime)
  double startTime = 0.0;
  double endTime = 0.0;
  StateVector startState{};    // relative to bodyId
  StateVector endState{};      // relative to bodyId
  SegmentEnd endReason = SegmentEnd::Horizon;
  std::string nextBodyId;      // set for SoiExit / SoiEntry

  // Position/velocity on this segment at time t (relative to bodyId).
  StateVector StateAtTime(double t) const;
};

struct PatchedOptions {
  int maxSegments = 16;
  bool stopAtImpact = true;
  // Encounter search: minimum step as a fraction of (child SOI / relative speed).
  double minStepSoiFraction = 0.02;
  // Encounter search: maximum step as a fraction of the shorter of the two orbital periods.
  double maxStepPeriodFraction = 1.0 / 64.0;
  // Safety cap on encounter-search steps per child and segment.
  int maxSearchSteps = 2000000;
};

// `horizon` is a duration in seconds measured from s.t. The input state is first re-rooted to
// the body whose SOI it is actually in.
std::vector<PatchedSegment> PredictPatched(const StateVector& s, const System& system, double horizon,
                                           const PatchedOptions& options);

// Finds the first time in [tStart, tStop] at which a craft on `el` (around the parent of
// childId) enters the SOI of childId. If the craft starts inside the SOI, it must leave first.
std::optional<double> FindSoiEntry(const OrbitalElements& el, double mu, const System& system,
                                   const std::string& childId, double tStart, double tStop,
                                   const PatchedOptions& options = {});

const char* ToString(SegmentEnd e);

}  // namespace oc
