#include "orbitcore/Orbit.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace oc {

namespace {

constexpr double kInf = std::numeric_limits<double>::infinity();

struct PQ { Vec3d P, Q; };

// Perifocal basis: P towards periapsis, Q 90 degrees ahead in the orbital plane.
PQ PerifocalBasis(const OrbitalElements& el) {
  const double cO = std::cos(el.lan), sO = std::sin(el.lan);
  const double cw = std::cos(el.argPe), sw = std::sin(el.argPe);
  const double ci = std::cos(el.i), si = std::sin(el.i);
  PQ b;
  b.P = {cO * cw - sO * sw * ci, sO * cw + cO * sw * ci, sw * si};
  b.Q = {-cO * sw - sO * cw * ci, -sO * sw + cO * cw * ci, cw * si};
  return b;
}

double OneMinusESq(double e) { return (1.0 - e) * (1.0 + e); }

}  // namespace

bool IsFinite(const OrbitalElements& el) {
  return std::isfinite(el.a) && std::isfinite(el.e) && std::isfinite(el.i) && std::isfinite(el.lan) &&
         std::isfinite(el.argPe) && std::isfinite(el.meanAnomalyAtEpoch) && std::isfinite(el.epoch);
}

OrbitalElements ElementsFromState(const StateVector& s, double mu) {
  OrbitalElements el{};
  el.epoch = s.t;
  const Vec3d r = s.r, v = s.v;
  const double rm = Length(r);
  const double vm = Length(v);
  if (!(rm > 0.0) || !(mu > 0.0) || !IsFinite(r) || !IsFinite(v)) {
    // Degenerate input: return a tiny circular orbit description without NaNs.
    el.a = std::max(rm, 1.0);
    return el;
  }
  const Vec3d rHat = r / rm;
  const Vec3d h = Cross(r, v);
  const double hm = Length(h);
  const double energy = 0.5 * vm * vm - mu / rm;
  const bool radial = hm <= 1e-12 * rm * vm || hm == 0.0;

  Vec3d hHat;
  if (!radial) {
    hHat = h / hm;
  } else {
    // Any plane containing r will do; prefer one whose normal is close to +Z.
    const Vec3d axis = std::fabs(rHat.z) < 0.9 ? Vec3d{0, 0, 1} : Vec3d{1, 0, 0};
    const Vec3d inPlane = Normalize(Cross(axis, rHat));     // perpendicular to r
    hHat = Normalize(Cross(rHat, inPlane));
  }

  el.i = std::atan2(std::sqrt(hHat.x * hHat.x + hHat.y * hHat.y), hHat.z);
  const Vec3d node{-hHat.y, hHat.x, 0.0};
  const double nm = Length(node);
  const bool equatorial = nm < 1e-11;
  const Vec3d nHat = equatorial ? Vec3d{1, 0, 0} : node / nm;
  el.lan = equatorial ? 0.0 : WrapTwoPi(std::atan2(nHat.y, nHat.x));

  Vec3d eVec = radial ? -rHat : Cross(v, h) / mu - rHat;
  double e = Length(eVec);
  const bool circular = !radial && e < 1e-11;
  if (circular) e = 0.0;

  if (radial) {
    // Degenerate conic: represent as an extremely thin ellipse/hyperbola (periapsis ~ 0).
    e = (energy >= 0.0) ? 1.0 + kRadialGuard : 1.0 - kRadialGuard;
    double a = -mu / (2.0 * energy);
    const double aMax = 1e6 * rm;  // (near-)escape speed: cap |a| so periapsis stays ~0
    if (!std::isfinite(a) || std::fabs(a) > aMax) a = (energy >= 0.0 ? -aMax : aMax);
    el.a = a;
  } else if (std::fabs(e - 1.0) < kParabolicGuard) {
    // Keep away from exactly parabolic (a would be infinite).
    e = (e >= 1.0) ? 1.0 + kParabolicGuard : 1.0 - kParabolicGuard;
  }
  el.e = e;

  if (!radial) {
    const double p = hm * hm / mu;
    el.a = p / OneMinusESq(e);
  }

  const Vec3d eHat = circular ? nHat : Normalize(eVec);
  el.argPe = circular ? 0.0 : WrapTwoPi(std::atan2(Dot(hHat, Cross(nHat, eHat)), Dot(nHat, eHat)));

  const double rdotv = Dot(r, v);
  if (e < 1.0) {
    double E;
    if (radial) {
      E = std::atan2(rdotv / std::sqrt(mu * el.a), 1.0 - rm / el.a);
    } else {
      const double nu = std::atan2(Dot(hHat, Cross(eHat, r)), Dot(eHat, r));
      E = EccentricFromTrue(nu, e);
    }
    el.meanAnomalyAtEpoch = WrapPi(MeanFromEccentric(E, e));
  } else {
    double H;
    if (radial) {
      H = std::asinh(rdotv / (e * std::sqrt(mu * -el.a)));
    } else {
      const double nu = std::atan2(Dot(hHat, Cross(eHat, r)), Dot(eHat, r));
      H = HyperbolicFromTrue(nu, e);
    }
    el.meanAnomalyAtEpoch = MeanFromHyperbolic(H, e);
  }
  return el;
}

double MeanMotion(const OrbitalElements& el, double mu) {
  const double aa = std::fabs(el.a);
  return std::sqrt(mu / (aa * aa * aa));
}

double MeanAnomalyAt(const OrbitalElements& el, double mu, double t) {
  return el.meanAnomalyAtEpoch + MeanMotion(el, mu) * (t - el.epoch);
}

StateVector StateAt(const OrbitalElements& el, double mu, double t) {
  StateVector out{};
  out.t = t;
  const double e = el.e;
  double x, y, vx, vy;
  if (e < 1.0) {
    const double a = el.a;
    const double M = WrapPi(MeanAnomalyAt(el, mu, t));
    const double E = SolveKeplerElliptic(M, e);
    const double sE = std::sin(E), cE = std::cos(E);
    const double sh = std::sin(0.5 * E);
    const double omc = 2.0 * sh * sh;  // 1 - cos E, no cancellation
    const double sq = std::sqrt(OneMinusESq(e));
    const double r = a * ((1.0 - e) + e * omc);
    x = a * ((1.0 - e) - omc);
    y = a * sq * sE;
    const double k = std::sqrt(mu * a) / r;
    vx = -k * sE;
    vy = k * sq * cE;
  } else {
    const double aa = -el.a;
    const double M = MeanAnomalyAt(el, mu, t);
    const double H = SolveKeplerHyperbolic(M, e);
    const double sH = std::sinh(H), cH = std::cosh(H);
    const double shh = std::sinh(0.5 * H);
    const double cm1 = 2.0 * shh * shh;  // cosh H - 1
    const double sq = std::sqrt((e - 1.0) * (e + 1.0));
    const double r = aa * ((e - 1.0) + e * cm1);
    x = aa * ((e - 1.0) - cm1);
    y = aa * sq * sH;
    const double k = std::sqrt(mu * aa) / r;
    vx = -k * sH;
    vy = k * sq * cH;
  }
  const PQ b = PerifocalBasis(el);
  out.r = b.P * x + b.Q * y;
  out.v = b.P * vx + b.Q * vy;
  return out;
}

StateVector Propagate(const StateVector& s, double mu, double dt) {
  StateVector out = StateAt(ElementsFromState(s, mu), mu, s.t + dt);
  out.bodyId = s.bodyId;
  return out;
}

double Periapsis(const OrbitalElements& el) { return el.a * (1.0 - el.e); }
double Apoapsis(const OrbitalElements& el) { return el.e < 1.0 ? el.a * (1.0 + el.e) : kInf; }
double Period(const OrbitalElements& el, double mu) {
  return el.e < 1.0 ? kTwoPi / MeanMotion(el, mu) : kInf;
}
double SemiLatusRectum(const OrbitalElements& el) { return el.a * OneMinusESq(el.e); }
double SpecificEnergy(const OrbitalElements& el, double mu) { return -mu / (2.0 * el.a); }
double SpecificEnergy(const StateVector& s, double mu) {
  return 0.5 * LengthSq(s.v) - mu / Length(s.r);
}
Vec3d SpecificAngularMomentum(const StateVector& s) { return Cross(s.r, s.v); }

double OrbitalSpeed(double r, double a, double mu) {
  return std::sqrt(std::max(0.0, mu * (2.0 / r - 1.0 / a)));
}
double SpeedAt(const OrbitalElements& el, double mu, double t) { return Length(StateAt(el, mu, t).v); }
double CircularSpeed(double r, double mu) { return std::sqrt(mu / r); }
double EscapeSpeed(double r, double mu) { return std::sqrt(2.0 * mu / r); }

double TrueAnomalyAt(const OrbitalElements& el, double mu, double t) {
  const double M = MeanAnomalyAt(el, mu, t);
  if (el.e < 1.0) return WrapTwoPi(TrueFromMean(WrapTwoPi(M), el.e));
  return TrueFromMean(M, el.e);
}

std::optional<double> TimeAtTrueAnomaly(const OrbitalElements& el, double mu, double nu, double tAfter) {
  const double n = MeanMotion(el, mu);
  if (!(n > 0.0)) return std::nullopt;
  if (el.e < 1.0) {
    const double Mt = WrapTwoPi(MeanFromTrue(WrapPi(nu), el.e));
    const double Mnow = MeanAnomalyAt(el, mu, tAfter);
    const double dM = WrapTwoPi(Mt - Mnow);
    return tAfter + dM / n;
  }
  const double nuW = WrapPi(nu);
  if (1.0 + el.e * std::cos(nuW) <= 0.0) return std::nullopt;  // beyond asymptotes
  const double Mt = MeanFromTrue(nuW, el.e);
  const double t = el.epoch + (Mt - el.meanAnomalyAtEpoch) / n;
  if (t < tAfter) return std::nullopt;
  return t;
}

std::optional<double> NextTimeAtRadius(const OrbitalElements& el, double mu, double radius,
                                       double tAfter, bool outbound) {
  if (!(radius > 0.0) || el.e <= 0.0) return std::nullopt;  // circular: |r| constant
  const double n = MeanMotion(el, mu);
  if (!(n > 0.0)) return std::nullopt;
  // Work with eccentric/hyperbolic anomaly: well-conditioned also for (near-)radial orbits.
  if (el.e < 1.0) {
    const double c = (1.0 - radius / el.a) / el.e;  // cos E
    if (c > 1.0 || c < -1.0) return std::nullopt;
    const double E = std::acos(c);
    const double Mt = WrapTwoPi(MeanFromEccentric(outbound ? E : -E, el.e));
    const double dM = WrapTwoPi(Mt - MeanAnomalyAt(el, mu, tAfter));
    return tAfter + dM / n;
  }
  const double c = (1.0 + radius / -el.a) / el.e;  // cosh H
  if (c < 1.0) return std::nullopt;
  const double H = std::acosh(c);
  const double Mt = MeanFromHyperbolic(outbound ? H : -H, el.e);
  const double t = el.epoch + (Mt - el.meanAnomalyAtEpoch) / n;
  if (t < tAfter) return std::nullopt;
  return t;
}

double TimeToPeriapsis(const OrbitalElements& el, double mu, double t) {
  const double n = MeanMotion(el, mu);
  const double M = MeanAnomalyAt(el, mu, t);
  if (el.e < 1.0) return WrapTwoPi(-M) / n;
  return -M / n;
}

Vec3d GravityAcceleration(const Vec3d& r, double mu) {
  const double r2 = LengthSq(r);
  if (!(r2 > 0.0)) return {0.0, 0.0, 0.0};
  const double rm = std::sqrt(r2);
  return r * (-mu / (r2 * rm));
}

}  // namespace oc
