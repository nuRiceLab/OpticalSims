//
// Created by ilker on 10/22/25.
//

#ifndef GDMLOPTICKS_STEPPINGACTION_HH
#define GDMLOPTICKS_STEPPINGACTION_HH
#include "G4UserSteppingAction.hh"
#include "AnalysisManagerHelper.hh"
class G4OpBoundaryProcess;
class SteppingAction : public G4UserSteppingAction
{
    public:
        // Const.
        SteppingAction();
        // Destr.
        ~SteppingAction();
        void UserSteppingAction(const G4Step*) override;
        G4OpBoundaryProcess*  GetOpticalBoundaryProcess();
        std::map<G4String,G4int> fDetectIds;

};

inline G4double EtoWavelength(G4double E)
{
    // input photon energy in eV
    // return wavelength in nm:
    // Wavelength = h*c/e
    return ((CLHEP::h_Planck*CLHEP::c_light) / (CLHEP::eV * CLHEP::nm)) / E;
}
#endif //GDMLOPTICKS_STEPPINGACTION_HH