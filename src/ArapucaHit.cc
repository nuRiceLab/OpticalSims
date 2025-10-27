//
// Created by ilker on 10/27/25.
//

#include "../include/ArapucaHit.hh"
#include "G4VisManager.hh"
#include "G4Circle.hh"
template <class type> class G4Allocator;
G4ThreadLocal G4Allocator<ArapucaHit>* ArapucaHitA=nullptr;

ArapucaHit::ArapucaHit()
  : G4VHit()
{}

ArapucaHit::~ArapucaHit()
{

}

ArapucaHit::ArapucaHit(unsigned iid, G4String iname, G4double iwave,
                     G4double itime, G4ThreeVector ipos,
                     G4ThreeVector idir, G4ThreeVector ipol)
  : G4VHit()
{
    fid     = iid;
    fname   = iname;
    fwave   = iwave;
    ft      = itime;
    fpos    = ipos;
    fdir    = idir;
    fpol    = ipol;
}ArapucaHit::ArapucaHit(const ArapucaHit& p)
  : G4VHit()
{
    fid     = p.fid;
    fname   = p.fname;
    fwave   = p.fwave;
    ft      = p.ft;
    fpos    = p.fpos;
    fdir    = p.fdir;
    fpol    = p.fpol;
}

const ArapucaHit& ArapucaHit::operator=(const ArapucaHit& p)
{
    fid     = p.fid;
    fname   = p.fname;
    fwave   = p.fwave;
    ft      = p.ft;
    fpos    = p.fpos;
    fdir    = p.fdir;
    fpol    = p.fpol;
    return *this;
}

G4bool ArapucaHit::operator==(const ArapucaHit& p) const
{
    return (this == &p) ? true : false;
}

void ArapucaHit::Draw()
{
    G4VVisManager* visMan = G4VVisManager::GetConcreteInstance();
    if(visMan)
    {
        G4Circle circle(fpos);
        circle.SetScreenSize(2.);
        circle.SetFillStyle(G4Circle::filled);
        G4Colour colour(0., 0., 1);
        G4VisAttributes attribs(colour);
        circle.SetVisAttributes(attribs);
        visMan->Draw(circle);
    }
}

void ArapucaHit::Print()
{
    G4cout << "------ Printing Hit Info ------"<<G4endl;
    G4cout << "Detector Name : " << fname << G4endl;
    G4cout << "Detector ID : " << fid << G4endl;
    G4cout << "Hit Position : " << fpos.getX() << " " << fpos.getY() << " " << fpos.getZ() << G4endl;
    G4cout << "Hit Time : " << ft << G4endl;
    G4cout << "Hit Wavelength : " << fwave << G4endl;
    G4cout << "Hit Polarization : " << fpol.getX() << " " << fpol.getY() << " " << fpol.getZ() << G4endl;
    G4cout << "Hit Direction : " << fdir.getX() << " " << fdir.getY() << " " << fdir.getZ() << G4endl;

}