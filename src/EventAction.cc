//
// Created by ilker on 10/22/25.
//

#include "EventAction.hh"
#include "G4Event.hh"
#include "include/config.h"
#include "globals.hh"
#include "G4Threading.hh"
#ifdef With_Opticks
    #include "SEvt.hh"
    #include "G4CXOpticks.hh"
namespace {G4Mutex opticks_mt=G4MUTEX_INITIALIZER;}
#endif
EventAction::EventAction(): G4UserEventAction() {}
EventAction::~EventAction(){}

void BeginEvent(const G4Event* event) {

 cout << "Begin event " << event->GetEventID() << endl;
}
void EndEvent(const G4Event* event) {}