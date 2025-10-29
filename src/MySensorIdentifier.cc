//
// Created by ilker on 10/28/25.
//

#include "MySensorIdentifier.hh"
#include "G4VPhysicalVolume.hh"
#include "G4LogicalVolume.hh"
#include "G4SDManager.hh"
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
    if (lv->GetSensitiveDetector()!=nullptr)
    {
        //std::cout << "Testing_GlobalIdentiy " << std::endl;
        std::string_view name = std::string_view (pv->GetName().c_str(),pv->GetName().size());
        std::vector<std::string_view> spfirst=Split(name,'_');
        std::vector<std::string_view> spsecond=Split(spfirst[1],'-');
        int first,second,third,sid=0;

        first=std::stoi(std::string(spsecond[2]));
        second=std::stoi(std::string(spsecond[1]));
        third=std::stoi(std::string(spsecond[0]));
        sid=third*(10*4)+second*4+first;
        return sid ;
    }

    return -1;

}
std::vector<std::string_view> MySensorIdentifier::Split(const std::string_view s,char del)
{
    std::vector< std::string_view > result;
    size_t start=0;
    while (true)
    {
        size_t pos=s.find(del,start);
        if (pos==std::string::npos)
        {
            result.emplace_back(s.substr(start));
            break;
        }
        result.emplace_back(s.substr(start,pos-start));
        start=pos+1;
    }
    return result;
}