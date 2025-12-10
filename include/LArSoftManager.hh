//
// Created by ilker parmaksiz on 11/24/25.
// This is for reading IonAndScint root file from LarSoft
//

#ifndef GDMLOPTICKS_ROOTMANAGER_HH
#define GDMLOPTICKS_ROOTMANAGER_HH
#include <G4String.hh>
#include "G4GenericMessenger.hh"
#include "G4Threading.hh"
#include "G4AutoLock.hh"
#include "G4Threading.hh"
class LArSoftManager
{
    public:

    static LArSoftManager* getInstance(){
        G4AutoLock lock(&mtx);
        if(instance== nullptr){
            instance = new LArSoftManager();
        }
        return instance;
    };


    void init(G4String FileName);
    private:
    LArSoftManager(){};
    static LArSoftManager * instance;
    static G4Mutex mtx;

};


#endif //GDMLOPTICKS_ROOTMANAGER_HH