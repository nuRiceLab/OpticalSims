//
// Created by rice on 11/24/25.
//

// This is generated for reading larsoft ionization and scintilation root file.
// simulate the optical photons in GPU using information provided by this root file
#include "LArSoftManager.hh"
#include "G4GenericMessenger.hh"
#include "TTreeReader.h"
#include "TFile.h"
#include "ROOT/RDataFrame.hxx"
#include "G4Exception.hh"
#include "G4ExceptionSeverity.hh"
#include "TROOT.h"

/// For Initial Testing
LArSoftManager * LArSoftManager::instance = nullptr;
G4Mutex LArSoftManager::mtx;




void LArSoftManager::init(G4String FileName)
{
    //ROOT::EnableImplicitMT(); // multi-threading
    // Ionization and Scintilation Module of LArSoft produce a root file that contains hits and number of photons

    // Try to open the file
    TFile* f = TFile::Open(FileName);
    if (!f || f->IsZombie()) {
        G4String err="Error: file " + FileName + " does not exist or is corrupted.";
        std::cerr << err << std::endl;
        G4Exception("ROOTManager::init",err,FatalException,"Checking to see if file exist");
    }
    f->Close();

   // "run:event:t:x:y:z:ds:e:trackid:pdg:e_deposit:n_electron:n_photon:scintyield");

    std::cout << "ROOTManager::init()" << std::endl;
    std::cout << "Reading file " << FileName << std::endl;
    ROOT::RDataFrame df("Events", FileName);
    G4String obj="sim::SimEnergyDeposits_IonAndScint__G4.obj.";

    auto fpdgs  = df.Take<int>(obj+"pdgCode");
    auto ftrackId  = df.Take<int>(obj+"trackid");
    auto fedeps = df.Take<float>(obj+"edep");
    auto fnphoton = df.Take<float>(obj+"numPhotons");
    auto fLightYields   = df.Take<float>(obj+"scintYieldRatio");
    auto fStartPos=df.Take<float>(obj+"endPos");
    auto fEndPos = df.Take<float>(obj+"StartPos");
    auto startTime=df.Take<float>(obj+"startTime");
    auto endTime = df.Take<float>(obj+"endTime");
    std::cout << "Size " << fpdgs->size() << std::endl;
    std::cout << "Pdg " << fpdgs->at(0) << std::endl;
}
