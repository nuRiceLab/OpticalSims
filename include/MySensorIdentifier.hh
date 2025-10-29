//
// Created by ilker on 10/28/25.
//

#ifndef GDMLOPTICKS_MYSENSORIDENTIFIER_HH
#define GDMLOPTICKS_MYSENSORIDENTIFIER_HH

#include "U4SensorIdentifier.h"
class MySensorIdentifier : public U4SensorIdentifier{
    int getGlobalIdentity(const G4VPhysicalVolume*,const G4VPhysicalVolume*) override;
    int getInstanceIdentity(const G4VPhysicalVolume* ) const override ;
};


#endif //GDMLOPTICKS_MYSENSORIDENTIFIER_HH
