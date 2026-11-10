// Two-body orbit functions: state <-> elements, propagation, derived quantities, gravity.
//
// Frame convention: right-handed, reference plane = XY, reference direction = +X, +Z = "north"
// (angular momentum of a prograde equatorial orbit). Conversion to Unreal's left-handed frame
// happens in the UE module OrbitwerkCore, not here.
#pragma once

#include <optional>

#include "orbitcore/Kepler.h"
#include "orbitcore/Types.h"
#include "orbitcore/Vec3.h"

namespace oc {

// Exactly parabolic (e == 1) and purely radial trajectories cannot be represented by (a, e).
// ElementsFromState nudges e by this amount away from 1 (see README "Grenzfälle").
constexpr double kParabolicGuard = 1e-13;
// Purely radial trajectories (r x v = 0) get e = 1 -/+ kRadialGuard (see README).
constexpr double kRadialGuard = 1e-15;

// Propagates a state by dt seconds on its two-body conic around `mu`. Keeps bodyId.
StateVector Propagate(const StateVector& s, double mu, double dt);

// --- derived quantities ----------------------------------------------------------
double Periapsis(const OrbitalElements& el);              // radius [m]
double Apoapsis(const OrbitalElements& el);               // radius [m], +inf for e >= 1
double Period(const OrbitalElements& el, double mu);      // [s], +inf for e >= 1
double MeanMotion(const OrbitalElements& el, double mu);  // [rad/s]
double SemiLatusRectum(const OrbitalElements& el);        // p = a(1-e^2) [m]
double SpecificEnergy(const OrbitalElements& el, double mu);  // -mu/(2a) [J/kg]
double SpecificEnergy(const StateVector& s, double mu);       // v^2/2 - mu/r
Vec3d  SpecificAngularMomentum(const StateVector& s);         // r x v
// Vis-viva orbital speed at radius r on an orbit with semi-major axis a [m/s].
double OrbitalSpeed(double r, double a, double mu);
// Speed on the orbit at time t [m/s].
double SpeedAt(const OrbitalElements& el, double mu, double t);
double CircularSpeed(double r, double mu);
double EscapeSpeed(double r, double mu);

double MeanAnomalyAt(const OrbitalElements& el, double mu, double t);
double TrueAnomalyAt(const OrbitalElements& el, double mu, double t);

// First time >= tAfter at which the body passes true anomaly nu.
// Hyperbola: nullopt if that point lies in the past (or beyond the asymptotes).
std::optional<double> TimeAtTrueAnomaly(const OrbitalElements& el, double mu, double nu, double tAfter);

// First time >= tAfter at which |r| == radius, on the outbound (dr/dt > 0) or inbound branch.
// nullopt if the orbit never reaches that radius (or only in the past, for a hyperbola).
std::optional<double> NextTimeAtRadius(const OrbitalElements& el, double mu, double radius,
                                       double tAfter, bool outbound);

// Time until next periapsis passage (ellipse) / until periapsis (hyperbola, negative if past).
double TimeToPeriapsis(const OrbitalElements& el, double mu, double t);

// Point-mass gravitational acceleration at position r relative to a body with parameter mu.
Vec3d GravityAcceleration(const Vec3d& r, double mu);

// True if every element is a finite number.
bool IsFinite(const OrbitalElements& el);

}  // namespace oc
