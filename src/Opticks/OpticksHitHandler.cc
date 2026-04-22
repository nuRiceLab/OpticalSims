//
// Created by ilker on 10/30/25.
//

#include "Opticks/OpticksHitHandler.hh"
#include "SEvt.hh"
#include "G4CXOpticks.hh"
#include "OpticksPhoton.hh"
#include "OpticksGenstep.h"
#include "QSim.hh"
#include "G4RunManager.hh"
#include "g4root.hh"
// Opticks Hit Collection
// Handles getting hits from opticks to a file
// Need to implement the backtracer for LArSoft in this class
OpticksHitHandler * OpticksHitHandler::instance = nullptr;
G4Mutex OpticksHitHandler::mtx;

void OpticksHitHandler::CollectHits() {
    //Collecting Opticks Photons
    SEvt* sev             = SEvt::Get_EGPU();
    sphoton::Get(sphits, sev->getHit());
    auto run= G4RunManager::GetRunManager();
    G4int eventID=run->GetCurrentEvent()->GetEventID();

    for (auto & hit : sphits){
        OpticksHit ohit= OpticksHit();
        ohit.hit_id=hit.index;
        ohit.sensor_id=hit.get_identity()-1;
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
	std::cout << "[OpticksHitHandler::CollectHits] Amount of Hits: " << hits.size() <<std::endl;
    // clear the hits
    sphits.clear();
    sphits.shrink_to_fit();
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
        analysisManager->FillNtupleFColumn(1,3,it.x);
        analysisManager->FillNtupleFColumn(1,4,it.y);
        analysisManager->FillNtupleFColumn(1,5,it.z);
        analysisManager->FillNtupleFColumn(1,6,it.time);
        analysisManager->FillNtupleFColumn(1,7,it.wavelength);
        analysisManager->FillNtupleIColumn(1,8,it.boundary);
        analysisManager->AddNtupleRow(1);
    }
   hits.clear();
   hits.shrink_to_fit();
   G4CXOpticks::Get()->reset(eventID);
   QSim::Get()->reset(eventID);
}


 std::vector<sphoton>& OpticksHitHandler::GetSphotons()
{
    return sphotons;
}

void OpticksHitHandler::setPrimPhotons(std::vector<sphoton> sphts){
	sphotons = sphts;
}
void OpticksHitHandler::PrepPrimPhotons(std::vector<sphoton> sphotons)
	{

		std::cout << " [OpticksHitHandler::PrepPrimPhotons] Setting Photons ...." << std::endl;
		size_t num_floats = sphotons.size()*17;
       	float* data = reinterpret_cast<float*>(sphotons.data());
       	NP* photons = NP::MakeFromValues<float>(data, num_floats);
       	photons->reshape({ static_cast<int64_t>(sphotons.size()), 17});
       	SEvt::SetInputPhoton(photons);
	}
void OpticksHitHandler::PrimPhotonBatcher(int eventID)
	{
       	std::cout << " [OpticksHitHandler::PrimPhotonBatcher] Deciding if Batching Needed ...." << std::endl;

       	auto sphotons = GetSphotons();

       	long unsigned int CollectedPhotons=sphotons.size();
       	long unsigned int maxPhoton=SEventConfig::MaxPhoton();
       	// Simulate in batch
       	if(CollectedPhotons>=maxPhoton)
		{
       		std::cout << "[OpticksHitHandler::PrimPhotonBatcher] Simulating in Batch Mode ...." << std::endl;
       		//Simulate();
       		for (std::size_t i =0 ; i < CollectedPhotons; i+=maxPhoton)
			{
				std::size_t end =std::min(i+maxPhoton,sphotons.size());
       			std::vector<sphoton> batch(
	   			std::make_move_iterator(sphotons.begin() + i),
	   			std::make_move_iterator(sphotons.begin() + end));
				PrepPrimPhotons(batch);
				Simulate(eventID);
			}
		}else
		{
			std::cout << "[OpticksHitHandler::PrimPhotonBatcher] Simulating Photons at Once ...." << std::endl;
			PrepPrimPhotons(sphotons);
       		Simulate(eventID);
		}
	}

	// initiate simulation
void OpticksHitHandler::Simulate(int eventID)
	{

       	G4CXOpticks * g4xc=G4CXOpticks::Get();
       	//Event id needed in here
       	std::cout << " [OpticksHitHandler::Simulate]: Simulating Photons Within GPU for EventID " << eventID << " ...."  << std::endl;
		std::cout << " [OpticksHitHandler::Simulate]: Photons Collected = " << GetSphotons().size() <<std::endl;
		g4xc->simulate(eventID,0);
       	cudaDeviceSynchronize();

		if(SEvt::GetNumHit(0)>0) CollectHits();
       	else std::cout << "[OpticksHitHandler::Simulate]: No Hits" << std::endl;

		//Event id needed here
       	g4xc->reset(eventID);
	}