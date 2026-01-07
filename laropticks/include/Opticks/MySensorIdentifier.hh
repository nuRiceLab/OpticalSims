//
// Created by ilker on 10/28/25.
//

#ifndef GDMLOPTICKS_MYSENSORIDENTIFIER_HH
#define GDMLOPTICKS_MYSENSORIDENTIFIER_HH
#include <string>
#include <vector>
#include <map>
#include "U4SensorIdentifier.h"
#include "G4String.hh"
class MySensorIdentifier : public U4SensorIdentifier{

public:
    MySensorIdentifier(std::map<G4String, G4int> &ids);
    ~MySensorIdentifier();
    virtual void setLevel(int _level);
    int getGlobalIdentity(const G4VPhysicalVolume*,const G4VPhysicalVolume*) override;
    int getInstanceIdentity(const G4VPhysicalVolume* ) const override ;
private:
    std::map<G4String,G4int> &fDetectIds;
    int ids=0;
};


#endif //GDMLOPTICKS_MYSENSORIDENTIFIER_HH
