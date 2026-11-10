// Kepler's equation and anomaly conversions (elliptic e<1 and hyperbolic e>1).
#pragma once

namespace oc {

constexpr double kPi = 3.14159265358979323846;
constexpr double kTwoPi = 2.0 * kPi;

// Wraps an angle to [0, 2pi).
double WrapTwoPi(double angle);
// Wraps an angle to [-pi, pi).
double WrapPi(double angle);

// Solves M = E - e*sin(E) for the eccentric anomaly E (0 <= e < 1).
// Safeguarded Newton (falls back to bisection), converges for every M and e, incl. e -> 1.
// Returns E in the same 2pi-"revolution" as M (E - M is in [-pi, pi]).
double SolveKeplerElliptic(double M, double e);

// Solves M = e*sinh(H) - H for the hyperbolic anomaly H (e > 1). Safeguarded Newton.
double SolveKeplerHyperbolic(double M, double e);

// Anomaly conversions. Elliptic functions require e<1, hyperbolic ones e>1.
double TrueFromEccentric(double E, double e);
double EccentricFromTrue(double nu, double e);
double MeanFromEccentric(double E, double e);   // cancellation-safe near e->1
double TrueFromHyperbolic(double H, double e);
double HyperbolicFromTrue(double nu, double e);
double MeanFromHyperbolic(double H, double e);  // cancellation-safe near e->1

// True anomaly -> mean anomaly and back, dispatching on e.
double MeanFromTrue(double nu, double e);
double TrueFromMean(double M, double e);

}  // namespace oc
