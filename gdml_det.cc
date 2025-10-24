//
// ********************************************************************
// * License and Disclaimer                                           *
// *                                                                  *
// * The  Geant4 software  is  copyright of the Copyright Holders  of *
// * the Geant4 Collaboration.  It is provided  under  the terms  and *
// * conditions of the Geant4 Software License,  included in the file *
// * LICENSE and available at  http://cern.ch/geant4/license .  These *
// * include a list of copyright holders.                             *
// *                                                                  *
// * Neither the authors of this software system, nor their employing *
// * institutes,nor the agencies providing financial support for this *
// * work  make  any representation or  warranty, express or implied, *
// * regarding  this  software system or assume any liability for its *
// * use.  Please see the license in the file  LICENSE  and URL above *
// * for the full disclaimer and the limitation of liability.         *
// *                                                                  *
// * This  code  implementation is the result of  the  scientific and *
// * technical work of the GEANT4 collaboration.                      *
// * By using,  copying,  modifying or  distributing the software (or *
// * any work based  on the software)  you  agree  to acknowledge its *
// * use  in  resulting  scientific  publications,  and indicate your *
// * acceptance of all terms of the Geant4 Software license.          *
// ********************************************************************
//
/// \file persistency/gdml//gdml_det.cc
/// \brief Main program of the persistency/gdml/ example
//
//
//
//
// --------------------------------------------------------------
//      GEANT 4 - gdml_det
//
// --------------------------------------------------------------

#include <vector>

#include "ActionInitialization.hh"
#include "DetectorConstruction.hh"
#include "SensitiveDetector.hh"

#include "G4RunManagerFactory.hh"

#include "G4UImanager.hh"
#include "G4VisExecutive.hh"
#include "G4UIExecutive.hh"
#include "G4GDMLParser.hh"
#include "ColorReader.hh"
// PhysicsList
#include "G4EmStandardPhysics_option4.hh"
#include "PhysicsList.hh"
// macro loader
#include  "include/config.h"

// Opticks related header files
#ifdef With_Opticks
#include "SEventConfig.hh"
#include "OPTICKS_LOG.hh"
#include <cuda_runtime.h>
#include "G4OpticalPhysicsOpticks.hh"
#else
#include "G4OpticalPhysics.hh"
#endif

int main(int argc,char **argv)
{
    // Opticks Initialization
  #ifdef With_Opticks
      int device;

      OPTICKS_LOG(argc,argv); // This is needed for opticks
      cudaDeviceSynchronize();
      SEventConfig::Initialize();
      cudaGetDevice(&device);
      std::cout<<"GPU Device ID "<<device<< std::endl;
      /*
        if(device == 1) { cudaSetDevice(0); }
        std::cout<<"Device "<<device<< std::endl;
      */
  #endif

   G4cout << G4endl;
   G4cout <<" Usage : " << G4endl;
   G4cout << "Interactive Mode : ./gdml_det i GDML/dune10kt_v5_refactored_1x2x6_nowires.gdml g04.mac"
          << G4endl;
   G4cout << "Batch Mode : ./gdml_det i GDML/dune10kt_v5_refactored_1x2x6_nowires.gdml g04.mac"
          << G4endl;

   if (argc<2)
   {
      G4cout << "Error! Mandatory input file is not specified!" << G4endl;
      G4cout << G4endl;
      return -1;
   }

   // Detect interactive mode (if only one argument) and define UI session
   auto * fReader=new ColorReader;
   auto parser= new G4GDMLParser(fReader);
   G4UIExecutive* ui = 0;
   if ( strcmp(argv[1],"i") == 0 ) {
     ui = new G4UIExecutive(argc, argv);
     parser->Read(argv[2],false);
   } else
   {
     parser->Read(argv[1],false);
   }

   auto* runManager = G4RunManagerFactory::CreateRunManager();

  // Physics list
  G4VModularPhysicsList* physics_list = new PhysicsList();

  #ifdef With_Opticks
    std::cout << "Defining Opticks Physics List" << std::endl;
    physics_list->RegisterPhysics(new G4OpticalPhysicsOpticks());
  #else
    physics_list->RegisterPhysics(new G4OpticalPhysics());
  #endif

   runManager->SetUserInitialization(new DetectorConstruction(parser));
   runManager->SetUserInitialization(physics_list);
   // User action initialization
   runManager->SetUserInitialization(new ActionInitialization());
   runManager->SetNumberOfThreads(1);
   runManager->Initialize();

   // Initialize visualization
   G4VisManager* visManager = new G4VisExecutive;
   visManager->Initialize();

   // Get the pointer to the User Interface manager
   G4UImanager* UImanager = G4UImanager::GetUIpointer();

   // Process macro or start UI session
   if ( ! ui )   // batch mode
   {
     G4String command = "/control/execute ";
     G4String fileName = argv[2];
     UImanager->ApplyCommand(command+fileName);
   }
   else           // interactive mode
   {
     G4String command = "/control/execute ";
     G4String fileName = argv[3];
     UImanager->ApplyCommand(command+fileName);

     UImanager->ApplyCommand("/control/execute vis.mac");
     ui->SessionStart();
     delete ui;
   }
   delete visManager;
   delete runManager;
   delete parser;
   delete fReader;
}
