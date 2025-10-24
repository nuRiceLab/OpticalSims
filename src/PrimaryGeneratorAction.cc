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
/// \file persistency/gdml//src/PrimaryGeneratorAction.cc
/// \brief Implementation of the PrimaryGeneratorAction class
//
//
//
//

#include "PrimaryGeneratorAction.hh"
#include "G4Event.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4AnalysisManager.hh"
#include "G4ParticleDefinition.hh"
#include "G4GeneralParticleSource.hh"
#include "G4SystemOfUnits.hh"
#include "G4AnalysisManager.hh"

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

PrimaryGeneratorAction::PrimaryGeneratorAction()
 : G4VUserPrimaryGeneratorAction(), 
   fParticleGun(0)
{
  G4int n_particle = 1;
  fParticleGun = new G4GeneralParticleSource();
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

PrimaryGeneratorAction::~PrimaryGeneratorAction()
{
  delete fParticleGun;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void PrimaryGeneratorAction::GeneratePrimaries(G4Event* anEvent)
{
  auto analysisManager =  G4AnalysisManager::Instance();
  analysisManager->FillNtupleSColumn(0,0,fParticleGun->GetParticleDefinition()->GetParticleName());
  analysisManager->FillNtupleIColumn(0,1,fParticleGun->GetParticleDefinition()->GetParticleDefinitionID());
  analysisManager->FillNtupleDColumn(0,2,fParticleGun->GetParticleEnergy());
  analysisManager->FillNtupleDColumn(0,3,fParticleGun->GetParticlePosition().x());
  analysisManager->FillNtupleDColumn(0,4,fParticleGun->GetParticlePosition().y());
  analysisManager->FillNtupleDColumn(0,5,fParticleGun->GetParticlePosition().z());
  analysisManager->FillNtupleDColumn(0,6,fParticleGun->GetParticleTime());
  analysisManager->FillNtupleDColumn(0,7,fParticleGun->GetParticleMomentumDirection().x());
  analysisManager->FillNtupleDColumn(0,8,fParticleGun->GetParticleMomentumDirection().y());
  analysisManager->FillNtupleDColumn(0,9,fParticleGun->GetParticleMomentumDirection().z());
  analysisManager->AddNtupleRow();
  fParticleGun->GeneratePrimaryVertex(anEvent);
  // Add Analysis Manager
}
