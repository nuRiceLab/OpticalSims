//
// Created by ilker on 10/22/25.
//

#ifndef GDMLOPTICKS_STEPPINGACTION_HH
#define GDMLOPTICKS_STEPPINGACTION_HH
#include "G4UserSteppingAction.hh"

class SteppingAction : public G4UserSteppingAction
{
    public:
        // Const.
        SteppingAction();
        // Destr.
        ~SteppingAction();
        void UserSteppingAction(const G4Step*) override;
};


#endif //GDMLOPTICKS_STEPPINGACTION_HH