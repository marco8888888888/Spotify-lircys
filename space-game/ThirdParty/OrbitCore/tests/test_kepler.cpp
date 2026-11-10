#include <cmath>

#include "TestUtil.h"

using namespace oc;

TEST_CASE("Elliptic Kepler solver satisfies the equation for many M and e") {
  const double es[] = {0.0, 1e-12, 0.01, 0.3, 0.7, 0.9, 0.99, 0.999999, 1.0 - 1e-12};
  for (double e : es) {
    for (int k = -200; k <= 200; ++k) {
      const double M = k * 0.0371;
      const double E = SolveKeplerElliptic(M, e);
      REQUIRE(std::isfinite(E));
      CHECK(std::fabs(E - e * std::sin(E) - M) < 1e-12);
    }
  }
}

TEST_CASE("Elliptic Kepler: tiny M near parabolic is accurate (relative)") {
  const double e = 1.0 - 1e-10;
  for (double M : {1e-15, 1e-12, 1e-9, 1e-6, 1e-3}) {
    const double E = SolveKeplerElliptic(M, e);
    CHECK(std::fabs(MeanFromEccentric(E, e) - M) <= 1e-12 * M);
  }
}

TEST_CASE("Hyperbolic Kepler solver satisfies the equation") {
  const double es[] = {1.0 + 1e-12, 1.0 + 1e-6, 1.01, 1.5, 2.0, 5.0, 50.0};
  for (double e : es) {
    for (double M : {-1e6, -1000.0, -10.0, -1.0, -1e-3, 0.0, 1e-9, 1e-3, 0.5, 1.0, 10.0, 1000.0, 1e6}) {
      const double H = SolveKeplerHyperbolic(M, e);
      REQUIRE(std::isfinite(H));
      const double f = MeanFromHyperbolic(H, e);
      CHECK(std::fabs(f - M) <= 1e-11 * std::max(1.0, std::fabs(M)));
    }
  }
}

TEST_CASE("Anomaly conversions round-trip") {
  for (double e : {0.0, 0.1, 0.5, 0.95}) {
    for (double nu = -3.0; nu <= 3.0; nu += 0.25) {
      CHECK(TrueFromMean(MeanFromTrue(nu, e), e) == doctest::Approx(nu).epsilon(1e-11));
    }
  }
  for (double e : {1.1, 2.0, 10.0}) {
    const double nuMax = std::acos(-1.0 / e) - 1e-3;
    for (double nu = -nuMax; nu <= nuMax; nu += nuMax / 7.0) {
      CHECK(TrueFromMean(MeanFromTrue(nu, e), e) == doctest::Approx(nu).epsilon(1e-10));
    }
  }
}

TEST_CASE("Wrap helpers") {
  CHECK(WrapTwoPi(-0.5) == doctest::Approx(kTwoPi - 0.5));
  CHECK(WrapTwoPi(7.0) == doctest::Approx(7.0 - kTwoPi));
  CHECK(WrapPi(4.0) == doctest::Approx(4.0 - kTwoPi));
  CHECK(WrapTwoPi(std::nan("")) == 0.0);
}
