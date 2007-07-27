/* -------------------------------------------------------------------------- *
 *                      SimTK Core: SimTK Simbody(tm)                         *
 * -------------------------------------------------------------------------- *
 * This is part of the SimTK Core biosimulation toolkit originating from      *
 * Simbios, the NIH National Center for Physics-Based Simulation of           *
 * Biological Structures at Stanford, funded under the NIH Roadmap for        *
 * Medical Research, grant U54 GM072970. See https://simtk.org.               *
 *                                                                            *
 * Portions copyright (c) 2007 Stanford University and the Authors.           *
 * Authors: Michael Sherman                                                   *
 * Contributors:                                                              *
 *                                                                            *
 * Permission is hereby granted, free of charge, to any person obtaining a    *
 * copy of this software and associated documentation files (the "Software"), *
 * to deal in the Software without restriction, including without limitation  *
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,   *
 * and/or sell copies of the Software, and to permit persons to whom the      *
 * Software is furnished to do so, subject to the following conditions:       *
 *                                                                            *
 * The above copyright notice and this permission notice shall be included in *
 * all copies or substantial portions of the Software.                        *
 *                                                                            *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR *
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   *
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    *
 * THE AUTHORS, CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,    *
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR      *
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE  *
 * USE OR OTHER DEALINGS IN THE SOFTWARE.                                     *
 * -------------------------------------------------------------------------- */

/**@file
 * This is a simple example of using a constraint.
 * Here we have two independent pendulums hanging from ground pins.
 * They can be connected by a spring or a distance constraint.
 */

#include "SimTKsimbody.h"

#include "SimTKcpodes/Integrator.h"

#include <cmath>
#include <cstdio>
#include <exception>
#include <vector>

using namespace std;
using namespace SimTK;

static const Real Pi      = (Real)SimTK_PI, 
                  Deg2Rad = (Real)SimTK_DEGREE_TO_RADIAN,
                  Rad2Deg = (Real)SimTK_RADIAN_TO_DEGREE;

static const Transform GroundFrame;

static const Real m = 1;   // kg
static const Real g = 9.8; // meters/s^2; apply in �y direction
static const Real d = 0.5; // meters

class ShermsForce : public GeneralForceElements::CustomForce {
public:
    ShermsForce(const MobilizedBody& b1, const MobilizedBody& b2) : body1(b1), body2(b2) { }
    ShermsForce* clone() const {return new ShermsForce(*this);}

    void calc(const SimbodyMatterSubsystem& matter, const State& state,
              Vector_<SpatialVec>& bodyForces,
              Vector_<Vec3>&       particleForces,
              Vector&              mobilityForces,
              Real&                pe) const
    {
        const Vec3& pos1 = body1.getBodyTransform(state).T();
        const Vec3& pos2 = body2.getBodyTransform(state).T();
        const Real d = (pos2-pos1).norm();
        const Real k = 1000, d0 = 1;
        const Vec3 f = k*(d-d0)*(pos2-pos1)/d;
        body1.applyBodyForce(state, SpatialVec(Vec3(0),  f), bodyForces);
        body2.applyBodyForce(state, SpatialVec(Vec3(0), -f), bodyForces);
    }
private:
    const MobilizedBody& body1;
    const MobilizedBody& body2;
};
//template <class E> Vector_<E>
//operator*(const VectorView_<E>& l, const typename CNT<E>::StdNumber& r) 
//  { return Vector_<E>(l)*=r; }

void ff(Vector& v) {
    v = 23.;
}
    

int main(int argc, char** argv) {
  try { // If anything goes wrong, an exception will be thrown.

        // CREATE MULTIBODY SYSTEM AND ITS SUBSYSTEMS
    MultibodySystem         mbs;

    SimbodyMatterSubsystem  twoPends(mbs);
    UniformGravitySubsystem gravity(mbs, Vec3(0, -g, 0));
    GeneralForceElements    forces(mbs);
    DecorationSubsystem     viz(mbs);

        // ADD BODIES AND THEIR MOBILIZERS
    Body::Rigid pendulumBody = Body::Rigid(MassProperties(m, Vec3(0), Inertia(1)))
                                  .addDecoration(Transform(), DecorativeBrick(Vec3(.1,.0667,.05)));

    MobilizedBody::Pin leftPendulum(twoPends.Ground(),
                                      Transform(Vec3(-1, 0, 0)),
                                    pendulumBody,
                                      Transform(Vec3(0, d, 0)));
/*
    MobilizedBody::Ball rightPendulum = MobilizedBody::Ball(twoPends.Ground(), pendulumBody)
                                         .setDefaultInboardFrame(Vec3(1,0,0))
                                         .setDefaultOutboardFrame(Vec3(0,d,0));
 */

    Vec3 radii(1/2.,1/3.,1/4.); radii*=.5;
    MobilizedBody::Ellipsoid rightPendulum(twoPends.Ground(), pendulumBody);
    rightPendulum.setDefaultRadii(radii)
                 .setDefaultInboardFrame(Transform(Rotation(),Vec3(1,0,0)))
                 .setDefaultOutboardFrame(Vec3(0,d,0));
    rightPendulum.addInboardDecoration(Transform(),DecorativeEllipsoid(rightPendulum.getDefaultRadii())
                                                      .setColor(Purple).setOpacity(.5));
    const Vec3 r=rightPendulum.getDefaultRadii();
    const Real minr = std::min(r[0],std::min(r[1],r[2]));
    const Real hw = minr/2;  // half width of follower plate in x
    const Real hh = minr/20; // half height of follower plate
    rightPendulum.addOutboardDecoration(Transform(Vec3(0,0,hh)), // raise up so bottom is on xy plane
                                        DecorativeBrick(Vec3(hw,2*hw/3.,hh)).setColor(Gray).setOpacity(1));

    //leftPendulum.addBodyDecoration(Transform(), DecorativeBrick().setOpacity(.2));
    //rightPendulum.addInboardDecoration(Transform(), DecorativeSphere(0.1).setColor(Yellow));
    //rightPendulum.addOutboardDecoration(Transform(), DecorativeLine());


    //rightPendulum.setDefaultAngle(20*Deg2Rad);
    rightPendulum.setDefaultRotation(Rotation::aboutAxis(60*Deg2Rad, Vec3(0,0,1)));

    // Beauty is in the eye of the beholder ...
    //viz.addBodyFixedDecoration(leftPendulum,  Transform(), DecorativeSphere(.1).setColor(Red));
    //viz.addBodyFixedDecoration(rightPendulum, Transform(), DecorativeSphere(.1).setColor(Blue));

        // OPTIONALLY TIE TOGETHER WITH SPRING/DAMPER OR DISTANCE CONSTRAINT

    const Real distance = 2;      // nominal length for spring; length for constraint
    const Real stiffness = 100;   // only if spring is used
    const Real damping   = 10;     //          "

    char c;
    cout << "Constraint, spring, or nothing? c/s/n"; cin >> c;

    if (c == 'c')         
        Constraint::Rod(leftPendulum, Vec3(0),
                        rightPendulum, Vec3(0),
                        distance);
    else if (c == 's') {
        forces.addTwoPointLinearSpring(leftPendulum, Vec3(0),
                                       rightPendulum, Vec3(0),
                                       stiffness, distance);
        forces.addTwoPointLinearDamper(leftPendulum, Vec3(0),
                                       rightPendulum, Vec3(0),
                                       damping);
    }

    // Add visualization line (orange=spring, black=constraint)
    if (c=='c' || c=='s')
        viz.addRubberBandLine(leftPendulum, Vec3(0),
                              rightPendulum, Vec3(0),
                              DecorativeLine().setColor(c=='c' ? Black : Orange).setLineThickness(4));

    //forces.addMobilityConstantForce(leftPendulum, 0, 20);
    //forces.addCustomForce(ShermsForce(leftPendulum,rightPendulum));
    //forces.addGlobalEnergyDrain(3);

    mbs.setHasTimeAdvancedEvents(State(),false);

    cout << "HAS TIME ADVANCED EVENTS=" << mbs.hasTimeAdvancedEvents(State()) << endl;

    State s = mbs.realizeTopology(); // returns a reference to the the default state
    //twoPends.setUseEulerAngles(s, true);
    mbs.realizeModel(s); // define appropriate states for this System

    VTKReporter display(mbs);

    mbs.realize(s, Stage::Position);
    display.report(s);
    cout << "q=" << s.getQ() << endl;
    cout << "T_MbM=" << rightPendulum.getMobilizerTransform(s).T() << endl;
    cout << "Default configuration shown. Ready? "; cin >> c;


    leftPendulum.setAngle(s, -60*Deg2Rad);

    rightPendulum.setQToFitTranslation(s, Vec3(0,1,0));
    //rightPendulum.setQToFitRotation(s, Rotation());


    //TODO
    //rightPendulum.setUToFitLinearVelocity(s, Vec3(1.1,0,1.2));

    rightPendulum.setUToFitAngularVelocity(s, Vec3(0,10,0));


    s.setTime(0);

    mbs.realize(s, Stage::Velocity);
    display.report(s);

    cout << "q=" << s.getQ() << endl;
    cout << "T_MbM=" << rightPendulum.getMobilizerTransform(s).T() << endl;
    cout << "v_MbM=" << rightPendulum.getMobilizerVelocity(s)[1] << endl;
    cout << "Unassembled configuration shown. Ready to assemble? "; cin >> c;


    // Create a study using the Runge Kutta Merson or CPODES integrator
    //RungeKuttaMerson myStudy(mbs, s);

    Integrator myStudy(mbs, Integrator::RungeKuttaMerson);

    //CPodesIntegrator myStudy(mbs, s);
    //ExplicitEuler myStudy(mbs, s);
    //myStudy.setMaximumStepSize(0.001);
    myStudy.setAccuracy(1e-2);
    //myStudy.setProjectEveryStep(true);
    //myStudy.setConstraintTolerance(1e-7);
    //myStudy.setAllowInterpolation(false);
    //myStudy.setMaximumStepSize(.1);

    const Real dt = .02; // output intervals
    const Real finalTime = 10;

    myStudy.setFinalTime(finalTime);

    // Peforms assembly if constraints are violated.
    myStudy.initialize(s);

    cout << "ACCURACY IN USE=" << myStudy.getAccuracyInUse() << endl;
    cout << "CTOL IN USE=" << myStudy.getConstraintToleranceInUse() << endl;
    cout << "TIMESCALE=" << myStudy.getTimeScaleInUse() << endl;
    cout << "Y WEIGHTS=" << myStudy.getStateWeightsInUse() << endl;
    cout << "1/CTOLS=" << myStudy.getConstraintWeightsInUse() << endl;

    {
        const State& s = myStudy.getState();
        display.report(s);
        cout << "q=" << s.getQ() << endl;
        cout << "T_MbM=" << rightPendulum.getMobilizerTransform(s).T() << endl;
        cout << "Assembled configuration shown. Ready to simulate? "; cin >> c;
    }

    Integrator::SuccessfulStepStatus status;
    int nextReport = 0;
    int nextScheduledEvent = 0;
    Real schedule[] = {1.234, 3.1415, 3.14159, 4.5, 9.090909, 100.};
    while ((status=myStudy.stepTo(nextReport*dt,schedule[nextScheduledEvent]))
           != Integrator::EndOfSimulation) 
    {
        const State& s = myStudy.getState();
        mbs.realize(s);
        printf("%5g %10.4g E=%10.8g h%3d=%g %s%s\n", s.getTime(), 
            leftPendulum.getAngle(s)*Rad2Deg,
            mbs.getEnergy(s), myStudy.getNStepsTaken(),
            myStudy.getPreviousStepSizeTaken(),
            Integrator::successfulStepStatusString(status).c_str(),
            myStudy.isStateInterpolated()?" (INTERP)":"");
        //printf("     qerr=%10.8g uerr=%10.8g uderr=%10.8g\n",
        //    twoPends.getQErr(s).normRMS(),
        //    twoPends.getUErr(s).normRMS(),
        //    twoPends.getUDotErr(s).normRMS());


        display.report(s);
        //if (s.getTime() >= finalTime)
           // break;

        //status = myStudy.stepTo(s.getTime() + dt);

        if (s.getTime() >= nextReport*dt) 
            ++nextReport;

        if (s.getTime() >= schedule[nextScheduledEvent])
            ++nextScheduledEvent;
    }

    printf("# STEPS/ATTEMPTS = %d/%d\n", myStudy.getNStepsTaken(), myStudy.getNStepsAttempted());
    printf("# ERR TEST FAILS = %d\n", myStudy.getNErrorTestFailures());

  } 
  catch (const exception& e) {
    printf("EXCEPTION THROWN: %s\n", e.what());
    exit(1);
  }
  catch (...) {
    printf("UNKNOWN EXCEPTION THROWN\n");
    exit(1);
  }

}

