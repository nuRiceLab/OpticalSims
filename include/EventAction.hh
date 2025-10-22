//
// Created by ilker on 10/22/25.
//

#ifndef GDMLOPTICKS_EVENTACTION_HH
#define GDMLOPTICKS_EVENTACTION_HH

#include "G4UserEventAction.hh"
#include "globals.hh"
class G4Event;

using namespace std;
class EventAction : public G4UserEventAction
{
public:
    EventAction();
    ~EventAction();
    void BeginEvent(const G4Event*);
    void EndEvent(const G4Event*);
private:
    chrono::time_point<chrono::high_resolution_clock> startTime;
};


#endif //GDMLOPTICKS_EVENTACTION_HH