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
#include "Randomize.hh"
#include "G4Event.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4AnalysisManager.hh"
#include "G4ParticleDefinition.hh"
#include "G4GeneralParticleSource.hh"
#include "G4SystemOfUnits.hh"
#include "G4GenericMessenger.hh"
#include "G4OpticalPhoton.hh"
#include "G4PrimaryParticle.hh"
#include "G4PhysicalConstants.hh"
#include "G4RandomTools.hh"
#include "TFile.h"
#include "TTreeReader.h"
#include "TTreeReaderValue.h"
#include <iostream>

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

PrimaryGeneratorAction::PrimaryGeneratorAction()
 : G4VUserPrimaryGeneratorAction(), 
   fParticleGun(0),fmsg(nullptr),fFileName(""),finitParticleType("GPS"),fAmount(100),fPosition(G4ThreeVector(0,0,0)),fMom(9.7*eV),fSigmaMom(0.1*eV)
{
  //G4int n_particle = 1;
  fParticleGun = new G4GeneralParticleSource();
  fmsg=new G4GenericMessenger(this,"/PrimaryGenerationAction/input/","");
  fmsg->DeclareProperty("type",finitParticleType,"Initial Particle Type: LArSoft or GPS (Default)");
  fmsg->DeclareProperty("file",fFileName,"File Name to Read");
  fmsg->DeclarePropertyWithUnit("pos","mm",fPosition,"Position of the particle gun (x,y,z) in mm");
  fmsg->DeclareProperty("momentum",fMom,"The mean momentum of the photon in ev");
  fmsg->DeclareProperty("sigma_momentum",fSigmaMom,"Spread of photonMomentum  in ev");
  fmsg->DeclareProperty("PhLinAmount",fPhotonAmount,"Defines the minimum, maximum, and step size for generating photons with linearly spaced energies (in MeV)");
  fmsg->DeclareProperty("phamount",fAmount,"Amount of particles to produce or the amount of repeats for the linearly spaced energies primary photons");
  simPhotonCPU = true;

#ifdef With_Opticks
  if((SEventConfig::IntegrationMode()==1)) simPhotonCPU=false;
#endif

}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

PrimaryGeneratorAction::~PrimaryGeneratorAction()
{
  delete fParticleGun;
  delete fmsg;
}

G4double PrimaryGeneratorAction::EnergyToWavelength(G4double energy)
{
    // energy should be in eV (or any Geant4 energy unit)
    G4double wavelength = (h_Planck * c_light) / energy;  // in Geant4 length units
    return wavelength / nm;  // convert to nm
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void PrimaryGeneratorAction::GeneratePrimaries(G4Event* anEvent)
{
  if (finitParticleType=="GPS"){
	auto analysisManager =  G4AnalysisManager::Instance();
    fParticleGun->GeneratePrimaryVertex(anEvent);
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
    analysisManager->FillNtupleIColumn(0,10,anEvent->GetEventID());
    analysisManager->AddNtupleRow(0);


    if((fParticleGun->GetParticleDefinition()->GetParticleName()!="opticalphoton")) return;
      // Clear the sphotons before the next event
      #ifdef With_Opticks
      if(sphotons.size()>0){
            sphotons.clear();
            sphotons.shrink_to_fit();
      }
      #endif


      G4PrimaryVertex* vertex = new G4PrimaryVertex(fParticleGun->GetParticlePosition(),fParticleGun->GetParticleTime());

      for (int i=0; i<fAmount;i++) // Produce specified amount of photons
      {
          G4PrimaryParticle* particle = new G4PrimaryParticle(G4OpticalPhoton::Definition());
          particle->SetKineticEnergy(fParticleGun->GetParticleEnergy());
          particle->SetMomentumDirection( fParticleGun->GetParticleMomentumDirection());
          particle->SetPolarization( fParticleGun->GetParticlePolarization());
          vertex->SetPrimary(particle);
          #ifdef With_Opticks

          if( (SEventConfig::IntegrationMode()==1) || (SEventConfig::IntegrationMode()==3))
          {
             // Produce photon on GPU with GPS
             sphoton spht;
             spht.zero();
	         spht.zero_flags();
      	     spht.set_flag(TORCH);
      	     spht.pos=make_float3(fParticleGun->GetParticlePosition().x(),fParticleGun->GetParticlePosition().y(),fParticleGun->GetParticlePosition().z());
             spht.pol=make_float3(fParticleGun->GetParticlePolarization().x(),fParticleGun->GetParticlePolarization().y(),fParticleGun->GetParticlePolarization().z());
             spht.mom=make_float3(fParticleGun->GetParticleMomentumDirection().x(),fParticleGun->GetParticleMomentumDirection().y(),fParticleGun->GetParticleMomentumDirection().z());
             spht.wavelength=(1240)/(fParticleGun->GetParticleEnergy()/eV); // nm
             spht.time=0;
		     sphotons.push_back(spht);
          }
          #endif
      }
      anEvent->AddPrimaryVertex( vertex );

   }else if (finitParticleType=="ROOT")
   {
	  if(!fFileName){
          std::cout << fFileName <<"  is not found" <<std::endl;
          return;
      }
      // Loading photon info from a ROOT file
      TFile file(fFileName);
	  std::cout << "[PrimaryGeneratorAction::GeneratePrimaries] Using Root for reading photon positions" <<std::endl;
	  std::cout << "[PrimaryGeneratorAction::GeneratePrimaries] Reading  " << fFileName << " ..."<< std::endl;

      TTreeReader reader("photon_gen", &file);
      TTreeReaderValue<int>fevtID(reader,"evtID");
  	  TTreeReaderValue<double>fx(reader,"x");
  	  TTreeReaderValue<double>fy(reader,"y");
  	  TTreeReaderValue<double>fz(reader,"z");
  	  TTreeReaderValue<double>ft(reader,"t");
	  TTreeReaderValue<double>fpx(reader,"px");
  	  TTreeReaderValue<double>fpy(reader,"py");
  	  TTreeReaderValue<double>fpz(reader,"pz");
 	  TTreeReaderValue<double>fmx(reader,"mx");
  	  TTreeReaderValue<double>fmy(reader,"my");
  	  TTreeReaderValue<double>fmz(reader,"mz");
  	  TTreeReaderValue<double>fwave(reader,"wavelength");
  	  TTreeReaderValue<double>fenergy(reader,"energy");

      #ifdef With_Opticks
      if((SEventConfig::IntegrationMode()==1)) simPhotonCPU=false;

      // Clear the sphotons before the next event
      if(sphotons.size()>0){
            sphotons.clear();
            sphotons.shrink_to_fit();
      }
     #endif


      // Produce Photons from a root file
      while (reader.Next())
      {
		//std::cout << "Event ID "<< anEvent->GetEventID() << " Event From File " <<*fevtID << std::endl;
        if(anEvent->GetEventID()!=(*fevtID)-1) continue;

        if(simPhotonCPU)
        {
			//std::cout << "[PrimaryGeneratorAction::GeneratePrimaries] Simulating Photons in Geant4 for Event ID "<<*fevtID << std::endl;
            G4PrimaryParticle* particle = new G4PrimaryParticle(G4OpticalPhoton::Definition());
            G4PrimaryVertex* vertex = new G4PrimaryVertex(G4ThreeVector((*fx)*cm,(*fy)*cm,(*fz)*cm),(*ft)*ns);
            particle->SetKineticEnergy((*fenergy)*eV);
            particle->SetMomentumDirection( G4ThreeVector(*fmx,*fmy,*fmz) );
            particle->SetPolarization(G4ThreeVector(*fpx,*fpy,*fpz));
            vertex->SetPrimary(particle);
            anEvent->AddPrimaryVertex( vertex );
        }
        #ifdef With_Opticks
          if(SEventConfig::IntegrationMode()==1 || SEventConfig::IntegrationMode()==3)
          {
			  //std::cout << "[PrimaryGeneratorAction::GeneratePrimaries] Simulating Photons in GPU for Event ID "<<*fevtID << std::endl;
              sphoton spht;
              spht.zero();
              spht.zero_flags();
              spht.set_flag(TORCH);
              spht.pos=make_float3((*fx)*10,(*fy)*10,(*fz)*10); // mm
              spht.mom=make_float3(*fmx,*fmy,*fmz);
              spht.pol=make_float3(*fpx,*fpy,*fpz);
              spht.wavelength=*fwave; // nm
              spht.time=*ft; //ns
              sphotons.push_back(spht);
          }
       #endif

     }
  // Add Analysis Manager
  }else if (finitParticleType=="Performance")
  {
	  GeneratePrimaryLinearly(anEvent);
  }


  #ifdef With_Opticks
	  if(sphotons.size()>0){
		 std::cout << "[PrimaryGeneratorAction::GeneratePrimaries]: Amount of Photons to simulate " << sphotons.size() << std::endl;
		 OpticksHitHandler *OpticksHandler = OpticksHitHandler::getInstance();
	     OpticksHandler->setPrimPhotons(sphotons);
	 } else std::cout << "[PrimaryGeneratorAction::GeneratePrimaries]: No photons to simulate ... "<< std::endl;
  #endif
}

std::vector<G4double> PrimaryGeneratorAction::linspace(G4double start, G4double end, G4int num, G4int factor)
{
    std::vector<G4double> values;
    values.reserve(num);

    if (num == 1) {
        values.push_back(start*factor);
        return values;
    }

    G4double step = (end - start) / (num - 1);

    for (int i = 0; i < num; ++i) {
        values.push_back((start + step * i)*factor);
    }
    return values;
}
void PrimaryGeneratorAction::SinglePhotonGenerator(G4Event* anEvent, PrimaryPhoton &pht)
{
	 if(simPhotonCPU)
        {
			//std::cout << "[PrimaryGeneratorAction::GeneratePrimaries] Simulating Photons in Geant4 for Event ID "<<*fevtID << std::endl;
            G4PrimaryParticle* particle = new G4PrimaryParticle(G4OpticalPhoton::Definition());
            G4PrimaryVertex* vertex = new G4PrimaryVertex(G4ThreeVector(pht.x,pht.y,pht.z),pht.t);
            particle->SetKineticEnergy(pht.e);
            particle->SetMomentumDirection( G4ThreeVector(pht.mx,pht.my,pht.mz) );
            particle->SetPolarization(G4ThreeVector(pht.px,pht.py,pht.pz));
            vertex->SetPrimary(particle);
            anEvent->AddPrimaryVertex( vertex );
        }
        #ifdef With_Opticks
          if(SEventConfig::IntegrationMode()==1 || SEventConfig::IntegrationMode()==3)
          {
			  //std::cout << "[PrimaryGeneratorAction::GeneratePrimaries] Simulating Photons in GPU for Event ID "<<*fevtID << std::endl;
              sphoton spht;
              spht.zero();
              spht.zero_flags();
              spht.set_flag(TORCH);
              spht.pos=make_float3(pht.x,pht.y,pht.z); // mm
              spht.mom=make_float3(pht.mx,pht.my,pht.mz);
              spht.pol=make_float3(pht.px,pht.py,pht.pz);
              spht.wavelength=EnergyToWavelength(pht.e); // nm
              spht.time=pht.t; //ns
              sphotons.push_back(spht);
          }
       #endif

}

void PrimaryGeneratorAction::GeneratePrimaryLinearly(G4Event * anEvent)
{
	G4int evtID = anEvent->GetEventID();
	std::vector<G4double> NPhotons=linspace(0.25, 25, 11, 1e6); // ToDo passs this from macro .

	unsigned idx   = evtID / fAmount;   // fRepeat set from macro
	if (idx >= NPhotons.size()) return;
	unsigned long long N = NPhotons[idx];
	for (unsigned long long i = 0; i < N; ++i)
	{
		G4ThreeVector mdir = G4RandomDirection(); // isotropic
		G4ThreeVector pdir = G4RandomDirection(); // isotropic
		PrimaryPhoton pht ;
		pht.x = fPosition.x()*cm;
		pht.y = fPosition.y()*cm;
		pht.z = fPosition.z()*cm;
		pht.t = 0;
		pht.mx = mdir.x();
		pht.my = mdir.y();
		pht.mz = mdir.z();
		pht.px = pdir.x();
		pht.py = pdir.y();
		pht.pz = pdir.z();
		pht.e  =  G4RandGauss::shoot(fMom, fSigmaMom)*eV;
		SinglePhotonGenerator(anEvent, pht);

	}
}



