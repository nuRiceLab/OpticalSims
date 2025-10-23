//
// Created by ilker on 10/22/25.
//

#ifndef GDMLOPTICKS_TRACKINGACTION_HH
#define GDMLOPTICKS_TRACKINGACTION_HH
#include "G4UserTrackingAction.hh"


class TrackingAction : public G4UserTrackingAction
{
    public:
        // Const.
        TrackingAction();

        //Destrt.
        ~TrackingAction();

        void PreUserTrackingAction(const G4Track*) override;
        void PostUserTrackingAction(const G4Track*) override;
};


#endif //GDMLOPTICKS_TRACKINGACTION_HH