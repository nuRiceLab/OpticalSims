//
// Created by ilker on 10/27/25.
//

#ifndef GDMLOPTICKS_ARAPUCAHIT_HH
#define GDMLOPTICKS_ARAPUCAHIT_HH
#include "G4VHit.hh"
#include "G4THitsCollection.hh"
#include "G4Allocator.hh"
#include "G4ThreeVector.hh"

class G4VHit;

class ArapucaHit : public G4VHit
{
    public:
    ArapucaHit();
    ~ArapucaHit();
    ArapucaHit(unsigned iid, G4String iname, G4double iwave, G4double itime,
      G4ThreeVector ipos, G4ThreeVector idire,G4ThreeVector ipol);
    ArapucaHit(const ArapucaHit&);
    const ArapucaHit& operator=(const ArapucaHit&);
    G4bool operator==(const ArapucaHit&) const;
    inline void* operator new(size_t);
    inline void operator delete(void*);
    void Draw() final;
    void Print();


    private:
    unsigned int fid{ 0 };
    G4String fname{"None"};
    G4double fwave{ 0 };
    G4double ft{ 0 };
    G4ThreeVector fpos{ 0, 0, 0 };
    G4ThreeVector fdir{ 0, 0, 0 };
    G4ThreeVector fpol{ 0, 0, 0 };

};

using ArapucaHitsCollection = G4THitsCollection<ArapucaHit>;
extern G4ThreadLocal G4Allocator<ArapucaHit>* ArapucaHitA;

inline void* ArapucaHit::operator new(size_t)
{
    if(!ArapucaHitA)
    {
        ArapucaHitA = new G4Allocator<ArapucaHit>;
    }
    return (void*) ArapucaHitA->MallocSingle();
}

inline void ArapucaHit::operator delete(void* aHit)
{
    ArapucaHitA->FreeSingle((ArapucaHit*) aHit);
}


#endif //GDMLOPTICKS_ARAPUCAHIT_HH