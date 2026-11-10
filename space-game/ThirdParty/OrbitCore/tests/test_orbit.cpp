#include <cmath>
#include <cstdint>
#include <vector>

#include "TestUtil.h"

using namespace oc;
using namespace octest;

namespace {

void CheckElementsFinite(const OrbitalElements& el) {
  CHECK(std::isfinite(el.a));
  CHECK(std::isfinite(el.e));
  CHECK(std::isfinite(el.i));
  CHECK(std::isfinite(el.lan));
  CHECK(std::isfinite(el.argPe));
  CHECK(std::isfinite(el.meanAnomalyAtEpoch));
}

// Round trip state -> elements -> state at the same time.
void CheckRoundTrip(const StateVector& s, double mu, double tolR, double tolV) {
  const OrbitalElements el = ElementsFromState(s, mu);
  CheckElementsFinite(el);
  const StateVector b = StateAt(el, mu, s.t);
  REQUIRE(Finite(b));
  CHECK(RelErr(b.r, s.r) < tolR);
  if (Length(s.v) > 0.0) CHECK(RelErr(b.v, s.v) < tolV);
  else CHECK(Length(b.v) < 1e-6);
}

// Classic RK4 integration of the two-body problem for cross-checking.
StateVector IntegrateRK4(StateVector s, double mu, double duration, int steps) {
  const double h = duration / steps;
  for (int k = 0; k < steps; ++k) {
    const Vec3d r = s.r, v = s.v;
    const Vec3d k1v = GravityAcceleration(r, mu), k1r = v;
    const Vec3d k2v = GravityAcceleration(r + k1r * (h / 2), mu), k2r = v + k1v * (h / 2);
    const Vec3d k3v = GravityAcceleration(r + k2r * (h / 2), mu), k3r = v + k2v * (h / 2);
    const Vec3d k4v = GravityAcceleration(r + k3r * h, mu), k4r = v + k3v * h;
    s.r = r + (k1r + k2r * 2.0 + k3r * 2.0 + k4r) * (h / 6.0);
    s.v = v + (k1v + k2v * 2.0 + k3v * 2.0 + k4v) * (h / 6.0);
    s.t += h;
  }
  return s;
}

struct Lcg {
  uint64_t x = 0x9E3779B97F4A7C15ull;
  double Next() {  // [0,1)
    x = x * 6364136223846793005ull + 1442695040888963407ull;
    return static_cast<double>(x >> 11) * (1.0 / 9007199254740992.0);
  }
  double Range(double lo, double hi) { return lo + (hi - lo) * Next(); }
};

}  // namespace

TEST_CASE("Circular equatorial orbit") {
  const double mu = kMuPlanet;
  const double r0 = 7.0e6;
  const double vc = CircularSpeed(r0, mu);
  const StateVector s{{r0, 0, 0}, {0, vc, 0}, 100.0, "planet"};
  const OrbitalElements el = ElementsFromState(s, mu);
  CHECK(el.a == doctest::Approx(r0).epsilon(1e-12));
  CHECK(el.e < 1e-11);
  CHECK(el.i < 1e-12);
  CHECK(Periapsis(el) == doctest::Approx(r0).epsilon(1e-12));
  CHECK(Apoapsis(el) == doctest::Approx(r0).epsilon(1e-12));
  const double T = Period(el, mu);
  CHECK(T == doctest::Approx(kTwoPi * std::sqrt(r0 * r0 * r0 / mu)).epsilon(1e-12));

  const StateVector q = StateAt(el, mu, s.t + T / 4.0);
  CHECK(q.r.x == doctest::Approx(0.0).epsilon(1e-9).scale(r0));
  CHECK(q.r.y == doctest::Approx(r0).epsilon(1e-10));
  CHECK(q.v.x == doctest::Approx(-vc).epsilon(1e-10));
  for (int k = 0; k < 50; ++k) {
    const StateVector p = StateAt(el, mu, s.t + k * 0.137 * T);
    CHECK(Length(p.r) == doctest::Approx(r0).epsilon(1e-12));
    CHECK(Length(p.v) == doctest::Approx(vc).epsilon(1e-12));
  }
  CHECK(OrbitalSpeed(r0, el.a, mu) == doctest::Approx(vc).epsilon(1e-12));
}

TEST_CASE("Inclined ellipse conserves energy and angular momentum over many orbits") {
  const double mu = kMuPlanet;
  const OrbitalElements el0{2.0e7, 0.7, 0.9, 1.2, 2.5, 0.3, 0.0};
  const StateVector s0 = StateAt(el0, mu, 0.0);
  REQUIRE(Finite(s0));
  const double E0 = SpecificEnergy(s0, mu);
  const Vec3d h0 = SpecificAngularMomentum(s0);
  const double T = Period(el0, mu);

  CHECK(E0 == doctest::Approx(SpecificEnergy(el0, mu)).epsilon(1e-12));
  CHECK(Periapsis(el0) == doctest::Approx(6.0e6));
  CHECK(Apoapsis(el0) == doctest::Approx(3.4e7));

  // Chained propagation (elements re-derived every step) over ~3000 orbits.
  StateVector s = s0;
  const double dt = 0.3713 * T;
  for (int k = 0; k < 8000; ++k) {
    s = Propagate(s, mu, dt);
    REQUIRE(Finite(s));
  }
  CHECK(s.t / T > 2900.0);
  CHECK(std::fabs(SpecificEnergy(s, mu) / E0 - 1.0) < 1e-9);
  CHECK(RelErr(SpecificAngularMomentum(s), h0) < 1e-9);

  // Direct evaluation after 1000 whole periods returns to the start.
  const StateVector back = StateAt(el0, mu, 1000.0 * T);
  CHECK(RelErr(back.r, s0.r) < 1e-8);
  CHECK(RelErr(back.v, s0.v) < 1e-8);

  // Energy/h at many points of a single orbit.
  for (int k = 0; k < 200; ++k) {
    const StateVector p = StateAt(el0, mu, k * T / 199.0 + 12.0);
    CHECK(std::fabs(SpecificEnergy(p, mu) / E0 - 1.0) < 1e-12);
    CHECK(RelErr(SpecificAngularMomentum(p), h0) < 1e-12);
    CHECK(Length(p.v) == doctest::Approx(OrbitalSpeed(Length(p.r), el0.a, mu)).epsilon(1e-10));
  }
}

TEST_CASE("Kepler propagation matches numerical integration") {
  const double mu = kMuPlanet;
  const OrbitalElements el{1.2e7, 0.4, 0.5, 0.3, 1.0, 0.0, 0.0};
  const StateVector s0 = StateAt(el, mu, 0.0);
  const double T = Period(el, mu);
  const StateVector num = IntegrateRK4(s0, mu, 0.8 * T, 40000);
  const StateVector ana = StateAt(el, mu, 0.8 * T);
  CHECK(RelErr(ana.r, num.r) < 1e-8);
  CHECK(RelErr(ana.v, num.v) < 1e-8);

  // Hyperbola as well.
  const StateVector h0{{7.0e6, 0, 0}, {0, 1.3 * EscapeSpeed(7.0e6, mu), 1000.0}, 0.0, "planet"};
  const OrbitalElements hel = ElementsFromState(h0, mu);
  const StateVector hnum = IntegrateRK4(h0, mu, 20000.0, 40000);
  const StateVector hana = StateAt(hel, mu, 20000.0);
  CHECK(RelErr(hana.r, hnum.r) < 1e-8);
  CHECK(RelErr(hana.v, hnum.v) < 1e-8);
}

TEST_CASE("Hyperbolic trajectory") {
  const double mu = kMuPlanet;
  const double r0 = 7.0e6;
  const StateVector s{{r0, 0, 0}, {0, 1.5 * EscapeSpeed(r0, mu), 0}, 50.0, "planet"};
  const OrbitalElements el = ElementsFromState(s, mu);
  CHECK(el.e > 1.0);
  CHECK(el.a < 0.0);
  CHECK(Periapsis(el) == doctest::Approx(r0).epsilon(1e-12));
  CHECK(std::isinf(Apoapsis(el)));
  CHECK(std::isinf(Period(el, mu)));
  // e for periapsis state: e = r v^2/mu - 1
  const double v2 = LengthSq(s.v);
  CHECK(el.e == doctest::Approx(r0 * v2 / mu - 1.0).epsilon(1e-12));

  const double E0 = SpecificEnergy(s, mu);
  const Vec3d h0 = SpecificAngularMomentum(s);
  double prevR = 0.0;
  for (int k = 0; k <= 100; ++k) {
    const double t = s.t + k * 3600.0;
    const StateVector p = StateAt(el, mu, t);
    REQUIRE(Finite(p));
    CHECK(Length(p.r) > prevR);
    prevR = Length(p.r);
    CHECK(std::fabs(SpecificEnergy(p, mu) / E0 - 1.0) < 1e-10);
    CHECK(RelErr(SpecificAngularMomentum(p), h0) < 1e-11);
    // backwards in time: symmetric (mirror y)
    const StateVector m = StateAt(el, mu, s.t - (t - s.t));
    CHECK(Length(m.r) == doctest::Approx(Length(p.r)).epsilon(1e-10));
  }
  // Asymptotic speed v_inf^2 = 2E
  const StateVector far = StateAt(el, mu, s.t + 1e9);
  CHECK(Length(far.v) == doctest::Approx(std::sqrt(2.0 * E0)).epsilon(1e-4));

  // Very long times stay finite.
  CHECK(Finite(StateAt(el, mu, 1e15)));
  CHECK(Finite(StateAt(el, mu, -1e15)));
}

TEST_CASE("Round trip state -> elements -> state (fuzz, incl. retrograde and hyperbolic)") {
  Lcg rng;
  const double mu = kMuPlanet;
  for (int k = 0; k < 5000; ++k) {
    const double rm = rng.Range(6.5e6, 5e8);
    const Vec3d dir = Normalize(Vec3d{rng.Range(-1, 1), rng.Range(-1, 1), rng.Range(-1, 1)});
    const double vesc = EscapeSpeed(rm, mu);
    const double vm = rng.Range(0.05, 2.5) * vesc;
    const Vec3d vdir = Normalize(Vec3d{rng.Range(-1, 1), rng.Range(-1, 1), rng.Range(-1, 1)});
    const StateVector s{dir * rm, vdir * vm, rng.Range(-1e7, 1e7), "planet"};
    CheckRoundTrip(s, mu, 1e-9, 1e-9);

    // Propagate forward and back again.
    const OrbitalElements el = ElementsFromState(s, mu);
    const double dt = rng.Range(-1e5, 1e5);
    const StateVector fwd = StateAt(el, mu, s.t + dt);
    REQUIRE(Finite(fwd));
    const StateVector back = Propagate(fwd, mu, -dt);
    CHECK(RelErr(back.r, s.r) < 1e-7);
  }
}

TEST_CASE("Edge cases: e ~ 0, i ~ 0, i ~ pi") {
  const double mu = kMuPlanet;
  const double r0 = 7.0e6, vc = CircularSpeed(r0, mu);
  SUBCASE("exactly circular, equatorial") {
    CheckRoundTrip({{r0, 0, 0}, {0, vc, 0}, 0.0, ""}, mu, 1e-13, 1e-13);
  }
  SUBCASE("almost circular") {
    for (double de : {1e-15, 1e-13, 1e-11, 1e-9, 1e-6}) {
      const StateVector s{{r0, 0, 0}, {0, vc * (1.0 + de), 0}, 0.0, ""};
      CheckRoundTrip(s, mu, 1e-11, 1e-11);
      const OrbitalElements el = ElementsFromState(s, mu);
      CHECK(el.e < 3e-6);
    }
  }
  SUBCASE("almost equatorial") {
    for (double di : {0.0, 1e-16, 1e-13, 1e-10, 1e-7}) {
      const StateVector s{{r0, 0, 0}, {0, 1.1 * vc * std::cos(di), 1.1 * vc * std::sin(di)}, 0.0, ""};
      CheckRoundTrip(s, mu, 1e-11, 1e-11);
      const StateVector s2{{0.6 * r0, 0.8 * r0, 0}, {-0.8 * vc, 0.6 * vc, vc * di}, 0.0, ""};
      CheckRoundTrip(s2, mu, 1e-11, 1e-11);
    }
  }
  SUBCASE("retrograde equatorial (i = pi)") {
    const StateVector s{{0.6 * r0, 0.8 * r0, 0}, {0.8 * 1.2 * vc, -0.6 * 1.2 * vc, 0}, 0.0, ""};
    const OrbitalElements el = ElementsFromState(s, mu);
    CHECK(el.i == doctest::Approx(kPi));
    CheckRoundTrip(s, mu, 1e-11, 1e-11);
  }
  SUBCASE("circular and polar") {
    CheckRoundTrip({{r0, 0, 0}, {0, 0, vc}, 0.0, ""}, mu, 1e-12, 1e-12);
  }
}

TEST_CASE("Edge cases: e ~ 1 (near-parabolic), parabolic, radial") {
  const double mu = kMuPlanet;
  const double r0 = 7.0e6;
  const double vesc = EscapeSpeed(r0, mu);
  SUBCASE("near-parabolic at periapsis") {
    for (double d : {-1e-6, -1e-9, -1e-12, 0.0, 1e-12, 1e-9, 1e-6}) {
      const StateVector s{{r0, 0, 0}, {0, vesc * (1.0 + d), 0}, 0.0, ""};
      CheckRoundTrip(s, mu, 1e-9, 1e-9);
      const OrbitalElements el = ElementsFromState(s, mu);
      CHECK(std::fabs(el.e - 1.0) < 5e-6);
      CHECK(Periapsis(el) == doctest::Approx(r0).epsilon(1e-9));
      // propagate a day and compare against the parabola energy (~0) / angular momentum
      const StateVector p = StateAt(el, mu, 86400.0);
      REQUIRE(Finite(p));
      CHECK(RelErr(SpecificAngularMomentum(p), SpecificAngularMomentum(s)) < 1e-6);
    }
  }
  SUBCASE("near-parabolic away from periapsis") {
    const StateVector s{{r0, 2.0 * r0, 3e5}, Normalize(Vec3d{-0.3, 1.0, 0.1}) * EscapeSpeed(Length(Vec3d{r0, 2.0 * r0, 3e5}), mu), 0.0, ""};
    CheckRoundTrip(s, mu, 1e-8, 1e-8);
  }
  SUBCASE("radial: straight up, straight down, at rest") {
    const Vec3d dir = Normalize(Vec3d{0.3, -0.5, 0.8});
    for (double vf : {0.0, 0.5, 0.99, 1.0, 1.5}) {
      for (double sgn : {1.0, -1.0}) {
        const StateVector s{dir * r0, dir * (sgn * vf * vesc), 0.0, ""};
        const OrbitalElements el = ElementsFromState(s, mu);
        CheckElementsFinite(el);
        const StateVector b = StateAt(el, mu, 0.0);
        REQUIRE(Finite(b));
        if (vf == 1.0) {
          // exactly radial AND exactly escape speed: |a| is capped (see README)
          CHECK(RelErr(b.r, s.r) < 1e-4);
          CHECK(Length(b.v - s.v) < 1e-4 * vesc);
        } else {
          CHECK(Length(b.r - s.r) < 5.0);  // metres
          CHECK(Length(b.v - s.v) < 1e-3 * vesc + 1e-3);
        }
      }
    }
  }
  SUBCASE("high eccentricity ellipse") {
    const OrbitalElements el{1e9, 1.0 - 1e-6, 0.2, 0.1, 0.4, 0.0, 0.0};
    const StateVector s0 = StateAt(el, mu, 0.0);
    CHECK(Length(s0.r) == doctest::Approx(Periapsis(el)).epsilon(1e-9));
    const double T = Period(el, mu);
    for (int k = 1; k < 100; ++k) {
      const StateVector p = StateAt(el, mu, k * T / 100.0);
      REQUIRE(Finite(p));
      CHECK(std::fabs(SpecificEnergy(p, mu) / SpecificEnergy(el, mu) - 1.0) < 1e-6);
    }
    CheckRoundTrip(StateAt(el, mu, 0.123 * T), mu, 1e-8, 1e-8);
  }
}

TEST_CASE("Degenerate input never yields NaN") {
  const OrbitalElements el = ElementsFromState({{0, 0, 0}, {0, 0, 0}, 0.0, ""}, kMuPlanet);
  CHECK(IsFinite(el));
  CHECK(Finite(StateAt(el, kMuPlanet, 10.0)));
  CHECK(IsFinite(GravityAcceleration({0, 0, 0}, kMuPlanet)));
}

TEST_CASE("Time at radius / true anomaly / periapsis") {
  const double mu = kMuPlanet;
  const OrbitalElements el{2.0e7, 0.5, 0.3, 0.0, 0.0, 0.0, 0.0};  // at periapsis at t=0
  const double T = Period(el, mu);
  CHECK(TimeToPeriapsis(el, mu, 0.25 * T) == doctest::Approx(0.75 * T));
  const auto ta = TimeAtTrueAnomaly(el, mu, kPi, 0.0);
  REQUIRE(ta);
  CHECK(*ta == doctest::Approx(0.5 * T));
  const double R = 2.0e7;
  const auto tOut = NextTimeAtRadius(el, mu, R, 0.0, true);
  const auto tIn = NextTimeAtRadius(el, mu, R, 0.0, false);
  REQUIRE(tOut);
  REQUIRE(tIn);
  CHECK(*tOut < 0.5 * T);
  CHECK(*tIn > 0.5 * T);
  CHECK(*tIn == doctest::Approx(T - *tOut));
  CHECK(Length(StateAt(el, mu, *tOut).r) == doctest::Approx(R).epsilon(1e-10));
  CHECK_FALSE(NextTimeAtRadius(el, mu, 1e9, 0.0, true));
  CHECK(TrueAnomalyAt(el, mu, 0.5 * T) == doctest::Approx(kPi));

  // Hyperbola: inbound crossing after periapsis does not exist.
  const StateVector hs{{7e6, 0, 0}, {0, 1.2 * EscapeSpeed(7e6, mu), 0}, 0.0, ""};
  const OrbitalElements hel = ElementsFromState(hs, mu);
  CHECK_FALSE(NextTimeAtRadius(hel, mu, 1e8, 1.0, false));
  const auto th = NextTimeAtRadius(hel, mu, 1e8, 0.0, true);
  REQUIRE(th);
  CHECK(Length(StateAt(hel, mu, *th).r) == doctest::Approx(1e8).epsilon(1e-10));
}

TEST_CASE("Gravity acceleration") {
  const Vec3d g = GravityAcceleration({kRadiusPlanet, 0, 0}, kMuPlanet);
  CHECK(g.x == doctest::Approx(-kMuPlanet / (kRadiusPlanet * kRadiusPlanet)));
  CHECK(g.y == 0.0);
  CHECK(g.z == 0.0);
}
