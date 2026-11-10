#include "orbitcore/Kepler.h"

#include <algorithm>
#include <cmath>

namespace oc {

namespace {

// x - sin(x), accurate for small |x| (avoids catastrophic cancellation).
double XMinusSin(double x) {
  if (std::fabs(x) < 0.5) {
    const double x2 = x * x;
    // x^3/3! - x^5/5! + x^7/7! - ...
    double term = x * x2 / 6.0;
    double sum = term;
    for (int k = 2; k < 12; ++k) {
      term *= -x2 / ((2.0 * k) * (2.0 * k + 1.0));
      sum += term;
    }
    return sum;
  }
  return x - std::sin(x);
}

// sinh(x) - x, accurate for small |x|.
double SinhMinusX(double x) {
  if (std::fabs(x) < 0.5) {
    const double x2 = x * x;
    double term = x * x2 / 6.0;
    double sum = term;
    for (int k = 2; k < 12; ++k) {
      term *= x2 / ((2.0 * k) * (2.0 * k + 1.0));
      sum += term;
    }
    return sum;
  }
  return std::sinh(x) - x;
}

}  // namespace

double WrapTwoPi(double angle) {
  if (!std::isfinite(angle)) return 0.0;
  if (angle >= 0.0 && angle < kTwoPi) return angle;
  double r = std::fmod(angle, kTwoPi);
  if (r < 0.0) r += kTwoPi;
  if (r >= kTwoPi) r = 0.0;
  return r;
}

double WrapPi(double angle) {
  if (angle >= -kPi && angle < kPi) return angle;
  double r = WrapTwoPi(angle + kPi) - kPi;
  return r;
}

double MeanFromEccentric(double E, double e) {
  // E - e sinE = (1-e) sinE + (E - sinE)
  return (1.0 - e) * std::sin(E) + XMinusSin(E);
}

double MeanFromHyperbolic(double H, double e) {
  // e sinhH - H = (e-1) sinhH + (sinhH - H)
  return (e - 1.0) * std::sinh(H) + SinhMinusX(H);
}

double SolveKeplerElliptic(double M, double e) {
  if (!std::isfinite(M)) return 0.0;
  e = std::clamp(e, 0.0, 1.0 - 1e-15);
  const double Mw = WrapPi(M);       // in [-pi, pi)
  const double offset = M - Mw;      // multiple of 2pi
  if (e == 0.0) return M;

  // f(E) = E - e sinE - Mw is monotone; root lies in [-pi, pi].
  double lo = -kPi, hi = kPi;
  double E;
  if (e < 0.8) {
    E = Mw + e * std::sin(Mw);
  } else {
    // Good start near periapsis for highly eccentric orbits: E^3/6 ~ M (when 1-e small).
    const double cbrtGuess = std::cbrt(6.0 * Mw);
    E = (std::fabs(Mw) < 0.3) ? cbrtGuess : (Mw >= 0.0 ? kPi : -kPi) * 0.9 + 0.1 * Mw;
    E = std::clamp(E, lo, hi);
  }

  for (int it = 0; it < 100; ++it) {
    const double f = MeanFromEccentric(E, e) - Mw;
    if (f > 0.0) hi = std::min(hi, E); else lo = std::max(lo, E);
    const double fp = 1.0 - e * std::cos(E);
    double En = (fp > 0.0) ? E - f / fp : 0.5 * (lo + hi);
    if (!(En > lo && En < hi)) En = 0.5 * (lo + hi);  // safeguard: bisection step
    const double dE = En - E;
    E = En;
    if (std::fabs(dE) <= 1e-15 * std::fabs(E) || std::fabs(dE) < 1e-300 || hi - lo <= 4e-16 * std::max(std::fabs(lo), std::fabs(hi))) break;
  }
  return E + offset;
}

double SolveKeplerHyperbolic(double M, double e) {
  if (!std::isfinite(M)) return 0.0;
  e = std::max(e, 1.0 + 1e-15);
  if (M == 0.0) return 0.0;
  const double sign = M < 0.0 ? -1.0 : 1.0;
  const double Ma = std::fabs(M);
  // f(H) = e sinhH - H - Ma, monotone increasing, root in [0, asinh(Ma/(e-1))].
  double lo = 0.0;
  double hi = std::asinh(Ma / (e - 1.0));
  double H;
  if (Ma > 6.0 * e) {
    H = std::log(2.0 * Ma / e);
  } else if (e < 1.6 || Ma < 1.0) {
    H = std::cbrt(6.0 * Ma);              // near-parabolic / small M start
    H = std::min(H, std::asinh(Ma / e) + 1.0);
  } else {
    H = std::asinh(Ma / e);
  }
  H = std::clamp(H, lo, hi);

  for (int it = 0; it < 200; ++it) {
    const double f = MeanFromHyperbolic(H, e) - Ma;
    if (f > 0.0) hi = std::min(hi, H); else lo = std::max(lo, H);
    const double fp = e * std::cosh(H) - 1.0;
    double Hn = (fp > 0.0) ? H - f / fp : 0.5 * (lo + hi);
    if (!(Hn > lo && Hn < hi)) Hn = 0.5 * (lo + hi);
    const double dH = Hn - H;
    H = Hn;
    if (std::fabs(dH) <= 1e-15 * std::fabs(H) || std::fabs(dH) < 1e-300 || hi - lo <= 4e-16 * hi) break;
  }
  return sign * H;
}

double TrueFromEccentric(double E, double e) {
  const double nu = std::atan2(std::sqrt((1.0 - e) * (1.0 + e)) * std::sin(E), std::cos(E) - e);
  // keep the same revolution as E
  return nu + (E - WrapPi(E));
}

double EccentricFromTrue(double nu, double e) {
  const double E = std::atan2(std::sqrt((1.0 - e) * (1.0 + e)) * std::sin(nu), e + std::cos(nu));
  return E + (nu - WrapPi(nu));
}

double TrueFromHyperbolic(double H, double e) {
  return std::atan2(std::sqrt((e - 1.0) * (e + 1.0)) * std::sinh(H), e - std::cosh(H));
}

double HyperbolicFromTrue(double nu, double e) {
  const double denom = 1.0 + e * std::cos(nu);
  // Outside the asymptotes the anomaly is not reachable; clamp to a large finite value.
  if (denom <= 1e-15) return (std::sin(nu) >= 0.0 ? 1.0 : -1.0) * 700.0;
  return std::asinh(std::sqrt((e - 1.0) * (e + 1.0)) * std::sin(nu) / denom);
}

double MeanFromTrue(double nu, double e) {
  if (e < 1.0) return MeanFromEccentric(EccentricFromTrue(nu, e), e);
  return MeanFromHyperbolic(HyperbolicFromTrue(nu, e), e);
}

double TrueFromMean(double M, double e) {
  if (e < 1.0) return TrueFromEccentric(SolveKeplerElliptic(M, e), e);
  return TrueFromHyperbolic(SolveKeplerHyperbolic(M, e), e);
}

}  // namespace oc
