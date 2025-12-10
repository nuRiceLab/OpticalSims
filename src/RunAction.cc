//
// Created by ilker on 10/22/25.
//
#include "RunAction.hh"
#include "G4Run.hh"
#include "G4AnalysisManager.hh"


RunAction::RunAction(): G4UserRunAction(),fmsg(nullptr),fFileName("out.csv"){
    //G4int n_particle = 1;

    fmsg=new G4GenericMessenger(this,"/RunAction/output/","");
    fmsg->DeclareProperty("file",fFileName,"File Name to Save");
}

RunAction::~RunAction() {

}

void RunAction::BeginOfRunAction(const G4Run* run) {

    // Get the analysis manager
    G4AnalysisManager* analysisManager = G4AnalysisManager::Instance();

    // Open an output file
    if (analysisManager)
        analysisManager->OpenFile(fFileName);
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
    analysisManager->CreateNtupleDColumn("x");
    analysisManager->CreateNtupleDColumn("y");
    analysisManager->CreateNtupleDColumn("z");
    analysisManager->CreateNtupleDColumn("t");
    analysisManager->CreateNtupleDColumn("wavelength");
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
    analysisManager->FinishNtuple();

    startTime = chrono::high_resolution_clock::now();
    RunTime =0;
    G4cout << "### Run started ###" << G4endl;

}


void RunAction::EndOfRunAction(const G4Run* run) {
    auto duration = chrono::high_resolution_clock::now() - startTime;
    auto RunTime = chrono::duration_cast<chrono::duration<double>>(duration).count();
    std::cout << "Run time: " << RunTime << " seconds" << G4endl;

    // Write and Close File
    auto analysisManager = G4AnalysisManager::Instance();
    if (analysisManager){
        cout << "Saving Events to " << analysisManager->GetFileName() <<" root file .." << G4endl;
        analysisManager->Write();
        analysisManager->CloseFile();
        analysisManager->Clear();
    }
 }