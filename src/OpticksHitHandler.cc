//
// Created by ilker on 10/30/25.
//

#include "OpticksHitHandler.hh"
#include "SEvt.hh"
#include "G4CXOpticks.hh"
#include "OpticksPhoton.hh"
#include "OpticksGenstep.h"
#include "QSim.hh"
#include "G4RunManager.hh"
#include "G4AnalysisManager.hh"
// Initialize Static Member
OpticksHitHandler * OpticksHitHandler::instance = nullptr;
G4Mutex OpticksHitHandler::mtx;

void OpticksHitHandler::CollectHits() {
    //Collecting Opticks Photons
    SEvt* sev             = SEvt::Get_EGPU();
    sphoton::Get(sphotons, sev->getHit());
    auto run= G4RunManager::GetRunManager();
    G4int eventID=run->GetCurrentEvent()->GetEventID();
    for (auto & hit : sphotons){
        OpticksHit ohit= OpticksHit();
        ohit.hit_id=hit.idx();
        ohit.sensor_id=hit.identity;
        ohit.x=hit.pos.x;
        ohit.y=hit.pos.y;
        ohit.z=hit.pos.z;
        ohit.polx=hit.pol.x;
        ohit.poly=hit.pol.y;
        ohit.polz=hit.pol.z;
        ohit.momx=hit.mom.x;
        ohit.momy=hit.mom.y;
        ohit.momz=hit.mom.z;
        ohit.time=hit.time;
        ohit.boundary=hit.boundary();
        ohit.wavelength=hit.wavelength;
        hits.push_back(ohit);
    }

    // clear the hits
    sphotons.clear();
    sphotons.shrink_to_fit();
    G4CXOpticks::Get()->reset(eventID);
    QSim::Get()->reset(eventID);
}

void OpticksHitHandler::SaveHits(){
    auto run= G4RunManager::GetRunManager();
    G4int eventID=run->GetCurrentEvent()->GetEventID();
    G4AnalysisManager * analysisManager= G4AnalysisManager::Instance();
    for (auto it : hits){
        analysisManager->FillNtupleIColumn(1,0,eventID);
        analysisManager->FillNtupleIColumn(1,1,it.hit_id);
        analysisManager->FillNtupleIColumn(1,2,it.sensor_id);
        analysisManager->FillNtupleDColumn(1,3,it.x);
        analysisManager->FillNtupleDColumn(1,4,it.y);
        analysisManager->FillNtupleDColumn(1,5,it.time);
        analysisManager->FillNtupleDColumn(1,6,it.wavelength);
        analysisManager->AddNtupleRow(1);
    }
   hits.clear();
   hits.shrink_to_fit();
   G4CXOpticks::Get()->reset(eventID);
   QSim::Get()->reset(eventID);
}
