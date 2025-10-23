//
// Created by ilker on 10/22/25.
//

#include "TrackingAction.hh"
#include "G4TrackingManager.hh"
#include "G4OpticalPhoton.hh"
TrackingAction::TrackingAction() :G4UserTrackingAction(){}
TrackingAction::~TrackingAction(){}


void TrackingAction::PreUserTrackingAction(const G4Track* tr)
{
    /*
    if (tr->GetParticleDefinition()==G4OpticalPhoton::Definition())
    {
        fpTrackingManager->SetStoreTrajectory(false);
    } else
    {
        fpTrackingManager->SetStoreTrajectory(true);
    }*/
    fpTrackingManager->SetStoreTrajectory(true);

}

void TrackingAction::PostUserTrackingAction(const G4Track* tr)
{

}
