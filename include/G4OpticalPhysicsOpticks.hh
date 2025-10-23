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
//
//
//---------------------------------------------------------------------------
//
// ClassName:   G4OpticalPhysics
//
// Author:      P.Gumplinger 30.09.2009
//
// Modified:    P.Gumplinger 29.09.2011
//              (based on code from I. Hrivnacova)
//
//
//              Modified by Ilker Parmaksiz on 3/5/25.
//              Opticks S1 Photons Implementation
//
////////////////////////////////////////////////////////////////////////
//---------------------------------------------------------------------------
//
// This class provides construction of default optical physics

#ifndef G4OpticalPhysics_h
#define G4OpticalPhysics_h 1

#include "G4VPhysicsConstructor.hh"
#include "G4OpticalParameters.hh"

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

class G4OpticalPhysicsOpticks : public G4VPhysicsConstructor
{
public:
    G4OpticalPhysicsOpticks(G4int verbose = 0, const G4String& name = "Optical");
    ~G4OpticalPhysicsOpticks() override;
    void PrintStatistics() const;

    G4OpticalPhysicsOpticks(const G4OpticalPhysicsOpticks& right) = delete;
    G4OpticalPhysicsOpticks& operator=(const G4OpticalPhysicsOpticks& right) = delete;

protected:
    // construct particle and physics
    void ConstructParticle() override;
    void ConstructProcess() override;

private:
    void PrintWarning(G4ExceptionDescription&) const;
};

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#endif  // G4OpticalPhysicsOpticks_h