//
// Created by ilker on 11/5/25.
//

#include <ArapucaHit.hh>

#include "G4Threading.hh"
#include "G4AutoLock.hh"
#include "G4ThreeVector.hh"

#ifndef GDMLOPTICKS_ANALYSISMANAGERHELPER_HH
#define GDMLOPTICKS_ANALYSISMANAGERHELPER_HH

#pragma once

class AnalysisManagerHelper;
extern thread_local std::unique_ptr<AnalysisManagerHelper> anaHelper;
class G4Step;
class AnalysisManagerHelper
{
    public:
        AnalysisManagerHelper();
        ~AnalysisManagerHelper();

        G4int GetG4ScintPhotons();
        G4int GetOpticksScintPhotons();
        G4int GetG4CerenkovPhotons();
        G4int GetOpticksCerenkovPhotons();
        G4int GetDuration();
        const std::map<G4String,G4int> & GetDetectIds();

        void AddG4ScintPhotons(G4int ph);
        void AddOpticksScintPhotons(G4int ph);
        void AddG4CerenkovPhotons(G4int ph);
        void AddOpticksCerenkovPhotons(G4int ph);
        void SetDuration(G4double dr);
        void SavePhotonInfotoFile();
        void SaveG4HitsToFile();
        void SetDetectIds(const std::map<G4String,G4int>  &fIDs);
        void SetBatchID(G4int id){fbatchID=id;};
        void AddG4Hits(ArapucaHit &hit);
        //void SetStartTime(std::chrono::high_resolution_clock::time_point time){fStartTime=time;};
        //std::chrono::high_resolution_clock::time_point GetStartTime(){return fStartTime;};
        void Reset();
        void SaveParticleSteps(const G4Step * step);
    private:

        G4int G4CerenkovPhotons{0};
        G4int OpticksCerenkovPhotons{0};
        G4int G4ScintPhotons{0};
        G4int OpticksScintPhotons{0};
        G4double Duration{0};
        std::map<G4String,G4int>  fDetectIds;
        std::vector<ArapucaHit> ArapucaHits{};
        std::chrono::high_resolution_clock::time_point fStartTime;
        G4int fbatchID;

};

inline void AnalysisManagerHelper::SetDetectIds(const std::map<G4String, G4int> &fIDs) {
    fDetectIds=fIDs;
}
inline void AnalysisManagerHelper::AddG4Hits(ArapucaHit &hit) {
    ArapucaHits.push_back(hit);
}

inline const std::map<G4String, G4int> &AnalysisManagerHelper::GetDetectIds() {
    return fDetectIds;
}
#endif //GDMLOPTICKS_ANALYSISMANAGERHELPER_HH