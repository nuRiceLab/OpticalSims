//
// Created by ilker on 10/30/25.
//

#ifndef GDMLOPTICKS_OPTICKSHITHANDLER_HH
#define GDMLOPTICKS_OPTICKSHITHANDLER_HH

#include "G4Threading.hh"
#include "G4AutoLock.hh"
#include "G4ThreeVector.hh"
// Opticks headers
#include "SEvt.hh"
#include "G4CXOpticks.hh"
#include "OpticksPhoton.hh"
#include "OpticksGenstep.h"
#include "QSim.hh"



class OpticksHitHandler {
    public:

        static OpticksHitHandler* getInstance(){
            G4AutoLock lock(&mtx);
            if(instance== nullptr){
                instance = new OpticksHitHandler();
            }
            return instance;
        };

        struct OpticksHit{
            G4int hit_id;
            G4int sensor_id;
            G4double time;
            G4double x,y,z;
            G4double momx,momy,momz;
            G4double polx,poly,polz;
            G4double wavelength;
            G4double boundary;
        };

        void CollectHits();
        void AddHits();
        void SaveHits();


    private:
        OpticksHitHandler(){};
        static OpticksHitHandler * instance;
        static G4Mutex mtx;
        std::vector<sphoton> sphotons;
        std::vector<OpticksHit> hits;



};



#endif //GDMLOPTICKS_OPTICKSHITHANDLER_HH
