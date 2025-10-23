//
// Created by ilker on 10/22/25.
//

#include "EventAction.hh"

#include <G4AutoLock.hh>

#include "G4Event.hh"
#include "include/config.h"
#include "globals.hh"
#include "G4Threading.hh"
#include "include/config.h"
#ifdef With_Opticks
    #include "SEvt.hh"
    #include "G4CXOpticks.hh"
namespace {G4Mutex opticks_mt=G4MUTEX_INITIALIZER;}
#endif
EventAction::EventAction(): G4UserEventAction() {}
EventAction::~EventAction(){}

void EventAction::BeginOfEventAction(const G4Event* event) {
 startTime = chrono::high_resolution_clock::now();
 cout << "Begin event " << event->GetEventID() << endl;
}
void EventAction::EndOfEventAction(const G4Event* event)
{
    G4int evtID=event->GetEventID();
#ifdef With_Opticks
    // Force Single Thread
    G4AutoLock lock(&opticks_mt);
    // Obtain predefined pointer to G4CXOpticks
    G4CXOpticks * g4cx=G4CXOpticks::Get();
    // Get event id and number of gensteps

    G4int ngenstep=SEvt::GetNumGenstepFromGenstep(0);
    G4int hits;

    // Simulate photons in opticks
    std::cout << "Number of GenStep " << ngenstep << std::endl;
    std::cout << "Number of Photons " << SEvt::GetNumPhotonCollected(0) << std::endl;

    if (ngenstep>0)
    {
        // Initiate the simulation in GPU
        g4cx->simulate(evtID, 0);
        cudaDeviceSynchronize();
        // get number of hits
        hits=SEvt::GetNumHit(0);
        if (hits>0) std::cout << hits << " hits" << std::endl;
        else std::cout << "No hits" << std::endl;
        g4cx->reset(evtID);
    }
#endif
    auto duration = chrono::high_resolution_clock::now() - startTime;
    auto EventTime = chrono::duration_cast<chrono::duration<double>>(duration).count();
    cout << "Event " <<  evtID <<", End Time " << EventTime << " seconds" << endl;

}