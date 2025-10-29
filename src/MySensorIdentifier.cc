//
// Created by ilker on 10/28/25.
//

#include "MySensorIdentifier.hh"
#include "G4VPhysicalVolume.hh"
int MySensorIdentifier::getInstanceIdentity(const G4VPhysicalVolume* pv ) const {
    // For instanced geometry, just return the copy number
    return pv->GetCopyNo();
}

int MySensorIdentifier::getGlobalIdentity(const G4VPhysicalVolume *pv, const G4VPhysicalVolume *ppv) {
    // Example: encode parent copyNo and child copyNo
    int parent = ppv ? ppv->GetCopyNo() : 0;
    int child  = pv->GetCopyNo();
    return parent * 10000 + child;
}