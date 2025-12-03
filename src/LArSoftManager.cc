//
// Created by rice on 11/24/25.
//

// This is generated for reading larsoft ionization and scintilation root file.
// simulate the optical photons in GPU using information provided by this root file
#include "../include/LArSoftManager.hh"
#include "G4GenericMessenger.hh"
#include "TTreeReader.h"
#include "TFile.h"
#include "ROOT/RDataFrame.hxx"
#include "G4Exception.hh"
#include "G4ExceptionSeverity.hh"
#include "TROOT.h"

LArSoftManager * LArSoftManager::instance = nullptr;
G4Mutex LArSoftManager::mtx;




void LArSoftManager::init()
{
    //ROOT::EnableImplicitMT(); // multi-threading
    // Ionization and Scintilation Module of LArSoft produce a root file that contains hits and number of photons

    // Try to open the file
    TFile* f = TFile::Open(fFileName);
    if (!f || f->IsZombie()) {
        G4String err="Error: file " + fFileName + " does not exist or is corrupted.";
        std::cerr << err << std::endl;
        G4Exception("ROOTManager::init",err,FatalException,"Checking to see if file exist");
    }
    f->Close();

   // "run:event:t:x:y:z:ds:e:trackid:pdg:e_deposit:n_electron:n_photon:scintyield");

    std::cout << "ROOTManager::init()" << std::endl;
    std::cout << "Reading file " << fFileName << std::endl;
    ROOT::RDataFrame df("is_ana/nt_is", fFileName);
    auto fRunIDs=df.Take<int>("run");
    auto feventIDs=df.Take<int>("event");
    auto fpdgs  = df.Take<int>("pdg");
    auto ftrackId  = df.Take<int>("trackid");
    auto fedeps = df.Take<float>("e_deposit");
    auto fnphoton = df.Take<float>("n_photon");
    auto fLightYields   = df.Take<float>("scintyield");
    auto fx=df.Take<float>("x");
    auto fy = df.Take<float>("y");
    auto fz = df.Take<float>("z");
    auto ft   = df.Take<float>("t");

}
