//
// Created by ilker parmaksiz on 11/24/25.
// This is for reading IonAndScint root file from LarSoft
//

#ifndef GDMLOPTICKS_ROOTMANAGER_HH
#define GDMLOPTICKS_ROOTMANAGER_HH
#include <G4String.hh>


class ROOTManager
{
    public:
    ROOTManager();
    ~ROOTManager();
    void OpenFile();
    void CloseFile();
    void ReadFile();
    private:
    G4String fileName;
};


#endif //GDMLOPTICKS_ROOTMANAGER_HH