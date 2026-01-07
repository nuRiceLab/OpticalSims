//
// Created by ilker on 10/22/25.
//

#ifndef GDMLOPTICKS_RUNACTION_HH
#define GDMLOPTICKS_RUNACTION_HH

#include "G4UserRunAction.hh"
#include "chrono"
#include "vector"
#include "G4GenericMessenger.hh"
using namespace std;

class RunAction:public G4UserRunAction
{
public:
  //Construct
  RunAction();
  // Destruct
  ~RunAction();
  void BeginOfRunAction(const G4Run*) override;
  void EndOfRunAction(const G4Run*) override;
  vector<double> RunTimes;
  vector<double> NPhotons;
  double RunTime;
  chrono::time_point<chrono::high_resolution_clock> startTime;
  G4GenericMessenger* fmsg;
  G4String fFileName;

};


#endif //GDMLOPTICKS_RUNACTION_HH