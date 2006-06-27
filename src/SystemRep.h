#ifndef SimTK_SIMBODY_SYSTEM_REP_H_
#define SimTK_SIMBODY_SYSTEM_REP_H_

/* Portions copyright (c) 2006 Stanford University and Michael Sherman.
 * Contributors:
 * 
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including 
 * without limitation the rights to use, copy, modify, merge, publish, 
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included 
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "SimTKcommon.h"
#include "simbody/internal/common.h"
#include "simbody/internal/State.h"
#include "simbody/internal/System.h"
#include "simbody/internal/Subsystem.h"

#include "SubsystemRep.h"


namespace SimTK {

class SystemRep {
public:
    SystemRep() 
      : systemName("<NONAME>"), systemVersion("0.0.0"), myHandle(0)
    {
        //takeOverSubsystem(0, DefaultSystemSubsystem());
    }
    SystemRep(int nSubsystems, const String& name, const String& version) 
      : systemName(name), systemVersion(version), subsystems(nSubsystems), myHandle(0)
    {
        //assert(nSubsystems >= 1);
        //takeOverSubsystem(0, DefaultSystemSubsystem());
    }
    virtual ~SystemRep() {
        clearMyHandle();
        subsystems.clear();
    }

    const String& getName()    const {return systemName;}
    const String& getVersion() const {return systemVersion;}

    int              getNSubsystems()    const {return subsystems.size();}
    const Subsystem& getSubsystem(int i) const {return subsystems[i];}
    Subsystem&       updSubsystem(int i)       {return subsystems[i];}

    SystemRep* clone() const {
        SystemRep* dup = cloneSystemRep();
        dup->myHandle = 0;
        return dup;
    }

	// Take over ownership from the Subsystem handle, install
    // it into a particular subsystem slot, and return the
	// new handle. This is only allowed if (a) the subsystem
    // slot is empty, and (b) the supplied Subsytem
	// already has a rep, but is NOT part of some other System.
	Subsystem& takeOverSubsystem(int subsys, Subsystem& src) {
        assert(0 <= subsys && subsys < getNSubsystems());
        assert(!subsystems[subsys].hasRep());
		assert(src.hasRep() && !src.isInSystem()); // TODO
        assert(src.isOwnerHandle());

		Subsystem& s = subsystems[subsys];
		s.setRep(src.updRep());			 // reference the passed-in rep
		s.updRep().setMyHandle(s);	     // steal ownership
        s.updRep().setSystem(*myHandle, subsys);
		return s;
	}

    virtual SystemRep* cloneSystemRep() const = 0;


    virtual void realizeConstruction(State& s) const { 
        for (int i=0; i<getNSubsystems(); ++i)
            subsystems[i].realize(s, Stage::Built);
    }
    virtual void realizeModeling(State& s) const {
        for (int i=0; i<getNSubsystems(); ++i)
            subsystems[i].realize(s, Stage::Modeled);
    }
    virtual void realizeParameters(const State& s) const { 
        for (int i=0; i<getNSubsystems(); ++i)
            subsystems[i].realize(s, Stage::Parametrized);
    }
    virtual void realizeTime(const State& s) const { 
        for (int i=0; i<getNSubsystems(); ++i)
            subsystems[i].realize(s, Stage::Timed);
    }
    virtual void realizeConfiguration(const State& s) const { 
        for (int i=0; i<getNSubsystems(); ++i)
            subsystems[i].realize(s, Stage::Configured);
    }
    virtual void realizeMotion(const State& s) const { 
        for (int i=0; i<getNSubsystems(); ++i)
            subsystems[i].realize(s, Stage::Moving);
    }
    virtual void realizeDynamics(const State& s) const { 
        for (int i=0; i<getNSubsystems(); ++i)
            subsystems[i].realize(s, Stage::Dynamics);
    }
    virtual void realizeReaction(const State& s) const { 
        for (int i=0; i<getNSubsystems(); ++i)
            subsystems[i].realize(s, Stage::Reacting);
    }

    void realize(const State& s, Stage g) const;

    virtual Real calcTimescale(const State& s) const {
        SimTK_STAGECHECK_GE(s.getSystemStage(), Stage::Parametrized,
            "System::calcTimescale()");
        return 0.1; // TODO!!!
    }

    // Default treats all state variable identically. Should be asking the 
    // subsystems. TODO
    virtual Real calcYErrorNorm(const State& s, const Vector& y_err) const {
        assert(y_err.size() == s.getY().size());
        SimTK_STAGECHECK_GE(s.getSystemStage(), Stage::Configured,
            "System::calcYErrorNorm()");
        return y_err.size()==0 ? 0 : std::sqrt( y_err.normSqr()/y_err.size() );
    }

    void setMyHandle(System& h) {myHandle = &h;}
    void clearMyHandle() {myHandle=0;}

protected:
    String systemName;
    String systemVersion;
	StableArray<Subsystem> subsystems;

private:
    friend class System;
    System* myHandle;     // the owner of this rep
};


class StudyRep {
public:
    StudyRep(const System& sys)
      : myHandle(0), system(new System(sys))
    {
        system->realize(state, Stage::Built);
    }

    virtual ~StudyRep() {
        delete system;
    }

    StudyRep* clone() const {
        StudyRep* dup = cloneStudyRep();
        dup->myHandle = 0;
        return dup;
    }
    virtual StudyRep* cloneStudyRep() const = 0;

    const System& getSystem() const {return *system;}
    const State&  getState()  const {return state;}
    State&        updState()        {return state;}

    void setMyHandle(Study& h) {myHandle = &h;}
    void clearMyHandle() {myHandle=0;}
private:
    friend class Study;
    Study* myHandle;     // the owner of this rep

    System* system;
    State   state;
};

} // namespace SimTK

#endif // SimTK_SIMBODY_SYSTEM_REP_H_
