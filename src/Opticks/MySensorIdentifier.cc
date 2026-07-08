//
// Created by ilker on 10/28/25.
//

#include "Opticks/MySensorIdentifier.hh"
#include "G4VPhysicalVolume.hh"
#include "G4LogicalVolume.hh"
#include "G4SDManager.hh"
MySensorIdentifier::MySensorIdentifier(std::map<G4String, G4int> &ids) : fDetectIds(ids) {

}
MySensorIdentifier::~MySensorIdentifier() {}
int MySensorIdentifier::getInstanceIdentity(const G4VPhysicalVolume* pv ) const {
    // For instanced geometry, just return the copy number

    if(fDetectIds.size()!=0){
        auto it =fDetectIds.find(pv->GetName());
        if(it != fDetectIds.end()){
            // Return the build detector id or just generate one
            std::cout << "Instance ID Found detector ID for " << pv->GetName() << ": " << it->second << std::endl;
            return (it->second > 0) ? it->second : pv->GetCopyNo();
        }
        return -1;
    }
    std::cout <<fDetectIds.size() <<std::endl;
    G4cout << " Could not find any detector IDs" << G4endl;
    //assert(false);
    return -1;

}

int MySensorIdentifier::getGlobalIdentity(const G4VPhysicalVolume *pv, const G4VPhysicalVolume *ppv) {

    if(fDetectIds.size()!=0){
        auto it =fDetectIds.find(pv->GetName());
        if(it != fDetectIds.end()){
            // Return the build detector id or just generate one
            //std::cout << "Global ID Found detector ID for " << pv->GetName() << ": " << it->second << std::endl;
            return (it->second > 0) ? it->second : ids++;
        }
        return -1;
    }
    std::cout <<fDetectIds.size() <<std::endl;
    G4cout << " Could not find any detector IDs" << G4endl;
    //assert(false);
    return -1;
}

void MySensorIdentifier::setLevel(int _level)
{
    
}
