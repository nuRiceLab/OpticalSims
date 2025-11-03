//
// Created by ilker on 10/22/25.
//
#include "G4OpticalPhoton.hh"
#include "G4OpBoundaryProcess.hh"
#include "G4ProcessManager.hh"
#include "SteppingAction.hh"
SteppingAction::SteppingAction():G4UserSteppingAction()
{

}

SteppingAction::~SteppingAction()
{

}

void SteppingAction::UserSteppingAction(const G4Step* step)
{
    /*
    auto track = step->GetTrack();
    G4ParticleDefinition* pdef = step->GetTrack()->GetDefinition();
    if (pdef != G4OpticalPhoton::Definition()) {return;}

    G4OpBoundaryProcess* boundary = nullptr;


    if (!boundary) { // the pointer is not defined yet
        // Get the list of processes defined for the optical photon
        // and loop through it to find the optical boundary process.
        G4ProcessVector* pv = pdef->GetProcessManager()->GetProcessList();
        for (size_t i=0; i<pv->size(); i++) {
            if ((*pv)[i]->GetProcessName() == "OpBoundary") {
                boundary = (G4OpBoundaryProcess*) (*pv)[i];
                break;
            }
        }
    }
        if (boundary) {
            G4OpBoundaryProcessStatus status = boundary->GetStatus();

            switch (status) {
            case Detection:
                G4cout << "Photon detected at boundary" << G4endl;
                break;
            case Absorption:
                G4cout << "Photon absorbed at boundary" << G4endl;
                break;
            case FresnelReflection:
                G4cout << "Photon reflected (Fresnel)" << G4endl;
                break;
            case TotalInternalReflection:
                G4cout << "Photon totally internally reflected" << G4endl;
                break;
            case Transmission:
                G4cout << "Photon transmitted through boundary" << G4endl;
                break;
            default:
                if (step->GetPostStepPoint()->GetMaterial()->GetName() != "LAr")
                {
                    G4cout << " Material Name " <<step->GetPostStepPoint()->GetMaterial()->GetName() << " Photon Status " << status  << G4endl;

                }
                break;
            }
        }*/

}
