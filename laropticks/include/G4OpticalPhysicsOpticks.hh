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
//---------------------------------------------------------------------------
//
// This class provides construction of default optical physics
//

#ifndef G4OpticalPhysics_h
#define G4OpticalPhysics_h 1

#include "G4OpticalProcessIndex.hh"
#include "G4OpticalPhysicsMessenger.hh"
#include "G4OpticalSurface.hh"

#include "G4VPhysicsConstructor.hh"
#include "globals.hh"

#include <vector>

class G4VProcess;
class G4EmSaturation;
class G4ScintillationOpticks;
class G4CerenkovOpticks;
class G4OpWLS;
class G4OpRayleigh;
class G4OpMieHG;
class G4OpBoundaryProcess;
class G4OpAbsorption;

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

class G4OpticalPhysicsOpticks : public G4VPhysicsConstructor
{
  public:

    G4OpticalPhysicsOpticks(G4int verbose = 0, const G4String& name = "Optical");
    virtual ~G4OpticalPhysicsOpticks();

  protected:

    // construct particle and physics
    virtual void ConstructParticle();
    virtual void ConstructProcess();

  private:

    /// Not implemented
    G4OpticalPhysicsOpticks(const G4OpticalPhysicsOpticks& right);
    /// Not implemented
    G4OpticalPhysicsOpticks& operator=(const G4OpticalPhysicsOpticks& right);

  public:

    // configure G4OpticalPhysics builder
    void Configure(G4OpticalProcessIndex, G4bool );

    void SetTrackSecondariesFirst(G4OpticalProcessIndex, G4bool );

    // Cerenkov
    void SetMaxNumPhotonsPerStep(G4int);
    void SetMaxBetaChangePerStep(G4double);
    void SetCerenkovStackPhotons(G4bool);
    void SetCerenkovTrackSecondariesFirst(G4bool);
    void SetCerenkovVerbosity(G4int);

    // Scintillation
    void SetScintillationYieldFactor(G4double );
    void SetScintillationExcitationRatio(G4double );
    void SetScintillationByParticleType(G4bool );
    void SetScintillationTrackInfo(G4bool );
    void SetScintillationTrackSecondariesFirst(G4bool);
    void SetFiniteRiseTime(G4bool );
    void SetScintillationStackPhotons(G4bool );
    void SetScintillationVerbosity(G4int);
    //void AddScintillationSaturation(G4EmSaturation* );

    // WLS
    void SetWLSTimeProfile(G4String );
    void SetWLSVerbosity(G4int);

    //boundary
    void SetBoundaryVerbosity(G4int);
    void SetInvokeSD(G4bool );

    void SetAbsorptionVerbosity(G4int);
    void SetRayleighVerbosity(G4int);
    void SetMieVerbosity(G4int);

  private:

    // methods
    void PrintStatistics() const;

    // messenger
    G4OpticalPhysicsMessenger* fMessenger;

    // The vector of process configuration
    std::vector<G4bool>         fProcessUse;

    // The vector of track secondaries options;
    // the option to track secondaries before finishing their parent track
    std::vector<G4bool>         fProcessTrackSecondariesFirst;

    // scintillation /////////////////
    static G4ThreadLocal G4ScintillationOpticks* fScintillationProcess;
    /// scintillation yield factor
    G4double                    fYieldFactor;

    /// scintillation excitation ratio
    G4double                    fExcitationRatio;

    /// option to set a finite rise-time; Note: the G4Scintillation
    /// process expects the user to have set the constant material
    /// property FAST/SLOWSCINTILLATIONRISETIME
    G4bool                      fFiniteRiseTime;

    /// option to  allow for the light yield to be a function of
    /// particle type and deposited energy in case of non-linear
    /// light emission in scintillators
    G4bool                      fScintillationByParticleType;

    /// option to allow for G4ScintillationTrackInformation
    /// to be attached to a scintillation photon's track
    G4bool                      fScintillationTrackInfo;

    /// option to allow stacking of secondary Scintillation photons
    G4bool                      fScintillationStackPhotons;

    G4int                       fScintillationVerbosity;

    ////////////////// Cerenkov
    static G4ThreadLocal G4CerenkovOpticks* fCerenkovProcess;
    /// max number of Cerenkov photons per step
    G4int                       fMaxNumPhotons;
    /// max change of beta per step
    G4double                    fMaxBetaChange;
    /// option to allow stacking of secondary Cerenkov photons
    G4bool                      fCerenkovStackPhotons;
    G4int                       fCerenkovVerbosity;

    ///////////////// WLS
    static G4ThreadLocal G4OpWLS* fWLSProcess;
    G4String                    fWLSTimeProfileName;
    G4int                       fWLSVerbosity;

    static G4ThreadLocal G4OpAbsorption* fAbsorptionProcess;
    G4int                       fAbsorptionVerbosity;

    static G4ThreadLocal G4OpRayleigh* fRayleighProcess;
    G4int                       fRayleighVerbosity;

    static G4ThreadLocal G4OpMieHG*                  fMieProcess;
    G4int                       fMieVerbosity;

    static G4ThreadLocal G4OpBoundaryProcess* fBoundaryProcess;
    /// G4OpBoundaryProcess to call InvokeSD method
    G4bool                      fInvokeSD;
    G4int                       fBoundaryVerbosity;
};

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#endif // G4OpticalPhysics_h
