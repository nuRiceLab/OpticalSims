//
// Created by ilker on 10/22/25.
//
#include "RunAction.hh"
#include "G4Run.hh"
#include "G4AnalysisManager.hh"
#include "AnalysisManagerHelper.hh"
#include "include/config.h"



RunAction::RunAction(): G4UserRunAction(),fmsg(nullptr),fFileName("out.root")
{
    //G4int n_particle = 1;

    fmsg=new G4GenericMessenger(this,"/RunAction/output/","");
    fmsg->DeclareProperty("file",fFileName,"File Name to Save");
}

RunAction::~RunAction() {
    delete fmsg;
}

void RunAction::BeginOfRunAction(const G4Run* run) {

    // Get the analysis manager
    auto analysisManager = G4AnalysisManager::Instance();
    analysisManager->SetNtupleMerging(true );          // 🔴 REQUIRED for MT merging
    analysisManager->SetFileName(fFileName);          // base name, no _t0 etc.


    // Open an output file

    cout << "Generating " << fFileName << G4endl;
    // Main Particle
    analysisManager->CreateNtuple("generator","Particle Generator Info");
    analysisManager->CreateNtupleSColumn("name");
    analysisManager->CreateNtupleIColumn("pdg");
    analysisManager->CreateNtupleDColumn("energy");
    analysisManager->CreateNtupleDColumn("ix");
    analysisManager->CreateNtupleDColumn("iy");
    analysisManager->CreateNtupleDColumn("iz");
    analysisManager->CreateNtupleDColumn("it");
    analysisManager->CreateNtupleDColumn("mx");
    analysisManager->CreateNtupleDColumn("my");
    analysisManager->CreateNtupleDColumn("mz");
    analysisManager->CreateNtupleIColumn("evtID");
    analysisManager->FinishNtuple();

    //Opticks Hits
    analysisManager->CreateNtuple("OpticksHits","Opticks Hits");
    analysisManager->CreateNtupleIColumn("evtID");
    analysisManager->CreateNtupleIColumn("hit_Id");
    analysisManager->CreateNtupleIColumn("SensorID");
    analysisManager->CreateNtupleFColumn("x");
    analysisManager->CreateNtupleFColumn("y");
    analysisManager->CreateNtupleFColumn("z");
    analysisManager->CreateNtupleFColumn("t");
    analysisManager->CreateNtupleFColumn("wavelength");
    analysisManager->CreateNtupleIColumn("boundary");
    analysisManager->FinishNtuple();

    //Geant4 Hits
    analysisManager->CreateNtuple("Geant4Hits","Geant4 Hits");
    analysisManager->CreateNtupleIColumn("evtID");
    analysisManager->CreateNtupleIColumn("SensorID");
    analysisManager->CreateNtupleSColumn("SensorName");
    analysisManager->CreateNtupleDColumn("x");
    analysisManager->CreateNtupleDColumn("y");
    analysisManager->CreateNtupleDColumn("z");
    analysisManager->CreateNtupleDColumn("t");
    analysisManager->CreateNtupleDColumn("wavelength");
    analysisManager->CreateNtupleIColumn("ProcessID");
    analysisManager->FinishNtuple();

    //PhotonInfo
    analysisManager->CreateNtuple("PhotonInfo","PhotonInfo");
    analysisManager->CreateNtupleIColumn("G4ScintPhotons");
    analysisManager->CreateNtupleIColumn("G4CernPhotons");
    analysisManager->CreateNtupleIColumn("OScintPhotons");
    analysisManager->CreateNtupleIColumn("OCerenkovPhotons");
    analysisManager->CreateNtupleDColumn("Time");
    analysisManager->CreateNtupleIColumn("eventID");
    analysisManager->CreateNtupleIColumn("BatchID");
    analysisManager->FinishNtuple();
#ifdef With_DEBUG
    //ParticleStep
    analysisManager->CreateNtuple("step","particle steps");
    analysisManager->CreateNtupleIColumn("eventID");
    analysisManager->CreateNtupleIColumn("trackID");
    analysisManager->CreateNtupleDColumn("x");
    analysisManager->CreateNtupleDColumn("y");
    analysisManager->CreateNtupleDColumn("z");
    analysisManager->CreateNtupleDColumn("t");
    analysisManager->CreateNtupleSColumn("volume1");
    analysisManager->CreateNtupleSColumn("volume2");
    analysisManager->CreateNtupleIColumn("status");
    analysisManager->CreateNtupleDColumn("mx");
    analysisManager->CreateNtupleDColumn("my");
    analysisManager->CreateNtupleDColumn("mz");
    analysisManager->CreateNtupleDColumn("px");
    analysisManager->CreateNtupleDColumn("py");
    analysisManager->CreateNtupleDColumn("pz");
    analysisManager->CreateNtupleDColumn("energy");
    analysisManager->FinishNtuple();
#endif

    analysisManager->OpenFile();
    startTime = chrono::high_resolution_clock::now();
    RunTime =0;
    G4cout << "### Run started ###" << G4endl;

}


void RunAction::EndOfRunAction(const G4Run* run) {
    auto duration = chrono::high_resolution_clock::now() - startTime;
    RunTime = chrono::duration_cast<chrono::duration<double>>(duration).count();
    if (G4Threading::IsMasterThread()) std::cout << "Run time: " << RunTime << " seconds" << G4endl;

    // Write and Close File
    auto analysisManager = G4AnalysisManager::Instance();
    if (analysisManager){
        if (G4Threading::IsMasterThread()) cout << "Saving Events to " << analysisManager->GetFileName() <<" root file .." << G4endl;
        analysisManager->Write();
        analysisManager->CloseFile();
    }
 }