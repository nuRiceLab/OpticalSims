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
     // Not using
    return -1;

}

int MySensorIdentifier::getGlobalIdentity(const G4VPhysicalVolume *pv, const G4VPhysicalVolume *ppv) {
    /*
    // Example: encode parent copyNo and child copyNo
    int parent = ppv ? ppv->GetCopyNo() : 0;
    int child  = pv->GetCopyNo();
    return parent * 10000 + child;
    */

    G4LogicalVolume *lv = pv->GetLogicalVolume();
    //std::cout << "Testing_GlobalIdentiy " << std::endl;
    /*std::string_view name = std::string_view (pv->GetName().c_str(),pv->GetName().size());
    std::vector<std::string_view> spfirst=Split(name,'_');
    std::vector<std::string_view> spsecond=Split(spfirst[1],'-');
    int first,second,third,sid=0;

    first=std::stoi(std::string(spsecond[2]));
    second=std::stoi(std::string(spsecond[1]));
    third=std::stoi(std::string(spsecond[0]));
    sid=third*(10*4)+second*4+first;
    */
    if(fDetectIds.size()!=0){
        auto it =fDetectIds.find(pv->GetName());
        if(it != fDetectIds.end()){
            return it->second ;
        }
        return -1;
    }
    std::cout <<fDetectIds.size() <<std::endl;
    G4cout << " Could not find the Detector ID " << G4endl;
    assert(false);

}
