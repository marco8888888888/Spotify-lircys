#include <cmath>
#include <stdexcept>
#include <string>

#include "TestUtil.h"

using namespace oc;
using namespace octest;

namespace {

Vec3d AbsPos(const System& sys, const StateVector& s) { return sys.Transform(s, sys.RootId()).r; }
Vec3d AbsVel(const System& sys, const StateVector& s) { return sys.Transform(s, sys.RootId()).v; }

// Craft at periapsis of a transfer ellipse in the planet frame, timed so that the apoapsis is
// reached `missDistance` short of the moon's orbit radius right when the moon is there.
StateVector MakeTransfer(const System& sys, double rp, double missDistance, double* transferTime) {
  const double ra = kMoonA - missDistance;
  const double aT = 0.5 * (rp + ra);
  const double tArr = kPi * std::sqrt(aT * aT * aT / kMuPlanet);
  const Vec3d moonAtArr = sys.BodyStateRelativeToParent("moon", tArr).r;
  const double phi = std::atan2(moonAtArr.y, moonAtArr.x) + kPi;
  const double vp = std::sqrt(kMuPlanet * (2.0 / rp - 1.0 / aT));
  if (transferTime) *transferTime = tArr;
  return StateVector{{rp * std::cos(phi), rp * std::sin(phi), 0}, {-vp * std::sin(phi), vp * std::cos(phi), 0}, 0.0, "planet"};
}

}  // namespace

TEST_CASE("System hierarchy, SOI radii and ephemerides") {
  const System sys = MakeTestSystem();
  std::string err;
  CHECK(sys.Validate(&err));
  CHECK(err.empty());
  CHECK(sys.RootId() == "sun");
  CHECK(sys.Depth("moon") == 2);
  REQUIRE(sys.ChildrenOf("planet").size() == 1);
  CHECK(sys.ChildrenOf("planet")[0] == "moon");

  const double soiMoon = sys.Get("moon").soiRadius;
  CHECK(soiMoon == doctest::Approx(kMoonA * std::pow(kMuMoon / kMuPlanet, 0.4)));
  CHECK(sys.Get("planet").soiRadius == doctest::Approx(kPlanetA * std::pow(kMuPlanet / kMuStar, 0.4)));
  CHECK_THROWS_AS(sys.Get("nope"), std::out_of_range);

  const double t = 123456.0;
  const StateVector moonAbs = sys.BodyStateAbsolute("moon", t);
  const StateVector planetAbs = sys.BodyStateAbsolute("planet", t);
  const StateVector moonRel = sys.BodyStateRelativeToParent("moon", t);
  CHECK(RelErr(moonAbs.r, planetAbs.r + moonRel.r) < 1e-15);
  CHECK(Length(moonRel.r) == doctest::Approx(kMoonA));
  CHECK(Length(planetAbs.r) == doctest::Approx(kPlanetA));

  // Transform round trip.
  const StateVector s{{1e7, 2e7, 3e6}, {100, -2000, 50}, t, "moon"};
  const StateVector p = sys.Transform(s, "planet");
  CHECK(p.bodyId == "planet");
  const StateVector back = sys.Transform(sys.Transform(p, "sun"), "moon");
  CHECK(Length(back.r - s.r) < 1e-3);
  CHECK(Length(back.v - s.v) < 1e-9);

  // Invalid systems.
  System bad;
  bad.AddBody(CelestialBody{"a", "A", "missing", 1.0, 1.0, 0.0, 0.0, OrbitalElements{1, 0, 0, 0, 0, 0, 0}, std::nullopt});
  CHECK_FALSE(bad.Validate(&err));
  CHECK_FALSE(err.empty());
}

TEST_CASE("Dominant body detection") {
  const System sys = MakeTestSystem();
  const double t = 5000.0;
  const Vec3d planet = sys.BodyStateAbsolute("planet", t).r;
  const Vec3d moon = sys.BodyStateAbsolute("moon", t).r;
  CHECK(sys.FindDominantBody(planet + Vec3d{7e6, 0, 0}, t) == "planet");
  CHECK(sys.FindDominantBody(moon + Vec3d{0, 2e6, 0}, t) == "moon");
  CHECK(sys.FindDominantBody(planet + Vec3d{0, 0, 2.0 * sys.Get("planet").soiRadius}, t) == "sun");

  // A state given in the wrong frame is re-rooted.
  const StateVector inSunFrame{moon + Vec3d{3e6, 0, 0}, sys.BodyStateAbsolute("moon", t).v, t, "sun"};
  const StateVector dom = sys.ToDominantBody(inSunFrame);
  CHECK(dom.bodyId == "moon");
  CHECK(Length(dom.r - Vec3d{3e6, 0, 0}) < 1e-3);
  CHECK(sys.IsOutsideSoi(StateVector{{2.0 * sys.Get("moon").soiRadius, 0, 0}, {0, 0, 0}, t, "moon"}));
}

TEST_CASE("PredictPatched: stable low orbit stays in one segment") {
  const System sys = MakeTestSystem();
  const double r0 = 7e6;
  const StateVector s{{r0, 0, 0}, {0, CircularSpeed(r0, kMuPlanet), 0}, 0.0, "planet"};
  const auto segs = PredictPatched(s, sys, 86400.0);
  REQUIRE(segs.size() == 1);
  CHECK(segs[0].bodyId == "planet");
  CHECK(segs[0].endReason == SegmentEnd::Horizon);
  CHECK(segs[0].endTime == doctest::Approx(86400.0));
  CHECK(Finite(segs[0].endState));
}

TEST_CASE("PredictPatched: SOI transition planet -> moon -> planet") {
  const System sys = MakeTestSystem();
  const double soiMoon = sys.Get("moon").soiRadius;
  double tArr = 0.0;
  const StateVector s = MakeTransfer(sys, 6.6e6, 2.0e7, &tArr);
  const auto segs = PredictPatched(s, sys, 30.0 * 86400.0);
  REQUIRE(segs.size() >= 2);
  INFO("segment 0 end: " << ToString(segs[0].endReason));

  CHECK(segs[0].bodyId == "planet");
  CHECK(segs[0].endReason == SegmentEnd::SoiEntry);
  CHECK(segs[0].nextBodyId == "moon");
  CHECK(segs[0].endTime < tArr);
  CHECK(segs[0].endTime > 0.5 * tArr);

  CHECK(segs[1].bodyId == "moon");
  CHECK(segs[1].startTime == doctest::Approx(segs[0].endTime));
  CHECK(Length(segs[1].startState.r) == doctest::Approx(soiMoon).epsilon(1e-6));
  CHECK(Length(segs[1].startState.r) <= soiMoon);
  CHECK(SpecificEnergy(segs[1].startState, kMuMoon) > 0.0);  // arrives on a hyperbola
  CHECK(segs[1].elements.e > 1.0);

  // Continuity across the patch (absolute frame).
  CHECK(Length(AbsPos(sys, segs[0].endState) - AbsPos(sys, segs[1].startState)) < 1e-3);
  CHECK(Length(AbsVel(sys, segs[0].endState) - AbsVel(sys, segs[1].startState)) < 1e-6);

  // Flyby: passes the moon above its surface and leaves again into the planet's SOI.
  CHECK(Periapsis(segs[1].elements) > kRadiusMoon);
  CHECK(segs[1].endReason == SegmentEnd::SoiExit);
  CHECK(segs[1].nextBodyId == "planet");
  REQUIRE(segs.size() >= 3);
  CHECK(segs[2].bodyId == "planet");
  CHECK(Length(segs[1].endState.r) == doctest::Approx(soiMoon).epsilon(1e-9));
  CHECK(Length(AbsPos(sys, segs[1].endState) - AbsPos(sys, segs[2].startState)) < 1e-3);
  // The flyby changed the planet-relative orbit.
  CHECK(std::fabs(segs[2].elements.a - segs[0].elements.a) > 1e6);

  for (const auto& seg : segs) {
    CHECK(seg.endTime >= seg.startTime);
    CHECK(IsFinite(seg.elements));
    CHECK(Finite(seg.startState));
    CHECK(Finite(seg.endState));
    // Each segment's conic reproduces its own end state.
    CHECK(Length(seg.StateAtTime(seg.endTime).r - seg.endState.r) < 1e-3);
  }
  CHECK(segs.back().endTime <= 30.0 * 86400.0 + 1e-6);
}

TEST_CASE("PredictPatched: direct hit on the moon ends with Impact") {
  const System sys = MakeTestSystem();
  // Search miss distances for one that hits the moon's surface (approach from inside the orbit).
  bool foundImpact = false;
  for (double miss = 0.0; miss < 6e6 && !foundImpact; miss += 2.5e5) {
    const StateVector s = MakeTransfer(sys, 6.6e6, -miss, nullptr);
    const auto segs = PredictPatched(s, sys, 20.0 * 86400.0);
    REQUIRE(!segs.empty());
    if (segs.size() >= 2 && segs[1].bodyId == "moon" && segs[1].endReason == SegmentEnd::Impact) {
      foundImpact = true;
      CHECK(Length(segs[1].endState.r) == doctest::Approx(kRadiusMoon).epsilon(1e-9));
      CHECK(Dot(segs[1].endState.r, segs[1].endState.v) < 0.0);
      CHECK(segs.size() == 2);
    }
  }
  CHECK(foundImpact);
}

TEST_CASE("PredictPatched: escape from planet into the star's SOI") {
  const System sys = MakeTestSystem();
  const double r0 = 7e6;
  // Escape along -Y so we stay away from the moon (moon starts on +X, moving +Y).
  const StateVector s{{0, r0, 0}, {-1.25 * EscapeSpeed(r0, kMuPlanet), 0, 2000.0}, 0.0, "planet"};
  const auto segs = PredictPatched(s, sys, 200.0 * 86400.0);
  REQUIRE(segs.size() >= 2);
  // May or may not meet the moon depending on geometry; the last transitions must reach the sun.
  bool reachedSun = false;
  for (size_t k = 0; k < segs.size(); ++k) {
    if (segs[k].endReason == SegmentEnd::SoiExit && segs[k].bodyId == "planet") {
      CHECK(segs[k].nextBodyId == "sun");
      REQUIRE(k + 1 < segs.size());
      CHECK(segs[k + 1].bodyId == "sun");
      CHECK(Length(segs[k].endState.r) == doctest::Approx(sys.Get("planet").soiRadius).epsilon(1e-9));
      reachedSun = true;
    }
  }
  CHECK(reachedSun);
  CHECK(segs.back().endReason == SegmentEnd::Horizon);
}

TEST_CASE("PredictPatched: suborbital hop impacts the planet") {
  const System sys = MakeTestSystem();
  const double r0 = kRadiusPlanet + 1000.0;
  const StateVector s{{r0, 0, 0}, {300.0, 1500.0, 0}, 10.0, "planet"};
  const auto segs = PredictPatched(s, sys, 86400.0);
  REQUIRE(segs.size() == 1);
  CHECK(segs[0].endReason == SegmentEnd::Impact);
  CHECK(segs[0].endTime > 10.0);
  CHECK(Length(segs[0].endState.r) == doctest::Approx(kRadiusPlanet).epsilon(1e-9));

  // Launch straight up from the surface (radial trajectory) also comes back down.
  const StateVector up{{kRadiusPlanet, 0, 0}, {2000.0, 0, 0}, 0.0, "planet"};
  const auto segsUp = PredictPatched(up, sys, 86400.0);
  REQUIRE(segsUp.size() == 1);
  CHECK(segsUp[0].endReason == SegmentEnd::Impact);
  CHECK(segsUp[0].endTime > 300.0);
  CHECK(Finite(segsUp[0].endState));
}

TEST_CASE("System gravity uses the dominant body") {
  const System sys = MakeTestSystem();
  const StateVector s{{kRadiusMoon, 0, 0}, {0, 0, 0}, 0.0, "moon"};
  const Vec3d g = sys.GravityAcceleration(s);
  CHECK(g.x == doctest::Approx(-kMuMoon / (kRadiusMoon * kRadiusMoon)));
}
