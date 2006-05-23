// @(#)root/eg:$Name:  $:$Id: TParticlePDG.cxx,v 1.9 2006/05/15 07:27:03 brun Exp $
// Author: Pasha Murat   12/02/99

#include "TDecayChannel.h"
#include "TParticlePDG.h"
#include "TDatabasePDG.h"

ClassImp(TParticlePDG)

//______________________________________________________________________________
TParticlePDG::TParticlePDG()
{
   //default constructor
   fPdgCode      = 0;
   fMass         = 0;
   fCharge       = 0;
   fLifetime     = 0;
   fWidth        = 0;
   fParity       = 0;
   fSpin         = 0;
   fIsospin      = 0;
   fI3           = 0;
   fStrangeness  = 0;
   fCharm        = 0;
   fBeauty       = 0;
   fTop          = 0;
   fY            = 0;
   fX            = 0;
   fStable       = 0;
   fDecayList    = 0;
   fTrackingCode = 0;
   fAntiParticle = 0;
}

//______________________________________________________________________________
TParticlePDG::TParticlePDG(Int_t )
{
   // empty for the time  being

   fPdgCode      = 0;
   fMass         = 0;
   fCharge       = 0;
   fLifetime     = 0;
   fWidth        = 0;
   fParity       = 0;
   fSpin         = 0;
   fIsospin      = 0;
   fI3           = 0;
   fStrangeness  = 0;
   fCharm        = 0;
   fBeauty       = 0;
   fTop          = 0;
   fY            = 0;
   fX            = 0;
   fStable       = 0;
   fDecayList    = 0;
   fTrackingCode = 0;
   fAntiParticle = 0;
}

//______________________________________________________________________________
TParticlePDG::TParticlePDG(const char* Name, const char* Title, Double_t Mass,
                           Bool_t Stable, Double_t Width, Double_t Charge,
                           const char* ParticleClass, Int_t PdgCode, Int_t Anti,
                           Int_t TrackingCode)
  : TNamed(Name,Title)
{

   // empty for the time  being
   fLifetime      = 0;
   fParity        = 0;
   fSpin          = 0;
   fIsospin       = 0;
   fI3            = 0;
   fStrangeness   = 0;
   fCharm         = 0;
   fBeauty        = 0;
   fTop           = 0;
   fY             = 0;
   fX             = 0;
   fStable        = 0;

   fMass          = Mass;
   fStable        = Stable;
   fWidth         = Width;
   fCharge        = Charge;
   fParticleClass = ParticleClass;
   fPdgCode       = PdgCode;
   fTrackingCode  = TrackingCode;
   fDecayList     = NULL;
   if (Anti) fAntiParticle = this;
   else      fAntiParticle = 0;
}

//______________________________________________________________________________
TParticlePDG::TParticlePDG(const TParticlePDG& pdg) :
  TNamed(pdg),
  fPdgCode(pdg.fPdgCode),
  fMass(pdg.fMass),
  fCharge(pdg.fCharge),
  fLifetime(pdg.fLifetime),
  fWidth(pdg.fWidth),
  fParity(pdg.fParity),
  fSpin(pdg.fSpin),
  fIsospin(pdg.fIsospin),
  fI3(pdg.fI3),
  fStrangeness(pdg.fStrangeness),
  fCharm(pdg.fCharm),
  fBeauty(pdg.fBeauty),
  fTop(pdg.fTop),
  fY(pdg.fY),
  fX(pdg.fX),
  fStable(pdg.fStable),
  fDecayList(pdg.fDecayList),
  fParticleClass(pdg.fParticleClass),
  fTrackingCode(pdg.fTrackingCode),
  fAntiParticle(pdg.fAntiParticle)
{ }

//______________________________________________________________________________
TParticlePDG& TParticlePDG::operator=(const TParticlePDG& pdg)
{
  if(this!=&pdg) {
    TNamed::operator=(pdg);
    fPdgCode=pdg.fPdgCode;
    fMass=pdg.fMass;
    fCharge=pdg.fCharge;
    fLifetime=pdg.fLifetime;
    fWidth=pdg.fWidth;
    fParity=pdg.fParity;
    fSpin=pdg.fSpin;
    fIsospin=pdg.fIsospin;
    fI3=pdg.fI3;
    fStrangeness=pdg.fStrangeness;
    fCharm=pdg.fCharm;
    fBeauty=pdg.fBeauty;
    fTop=pdg.fTop;
    fY=pdg.fY;
    fX=pdg.fX;
    fStable=pdg.fStable;
    fDecayList=pdg.fDecayList;
    fParticleClass=pdg.fParticleClass;
    fTrackingCode=pdg.fTrackingCode;
    fAntiParticle=pdg.fAntiParticle;
  } return *this;
}

//______________________________________________________________________________
TParticlePDG::~TParticlePDG() {
   //destructor
   if (fDecayList) {
      fDecayList->Delete();
      delete fDecayList;
   }
}


//______________________________________________________________________________
Int_t TParticlePDG::AddDecayChannel(Int_t        Type, 
                                    Double_t     BranchingRatio,
                                    Int_t        NDaughters, 
                                    Int_t*       DaughterPdgCode)
{
   // add new decay channel, Particle owns those...

   Int_t n = NDecayChannels();
   if (NDecayChannels() == 0) {
      fDecayList = new TObjArray(5);
   }
   TDecayChannel* dc = new TDecayChannel(n,Type,BranchingRatio,NDaughters,
                                        DaughterPdgCode);
   fDecayList->Add(dc);
   return 0;
}

//_____________________________________________________________________________
TDecayChannel* TParticlePDG::DecayChannel(Int_t i) 
{ 
   //return pointer to decay channel object at index i
   return (TDecayChannel*) fDecayList->At(i); 
}

//_____________________________________________________________________________
void TParticlePDG::PrintDecayChannel(TDecayChannel* dc, Option_t* option) const
{
   //print the list of decays
   if (strstr(option,"banner")) {
                                // print banner

      printf(" Channel Code BranchingRatio Nd  ");
      printf(" ...................Daughters.................... \n");
   }
   if (strstr(option,"data")) {

      TDatabasePDG* db = TDatabasePDG::Instance();

      printf("%7i %5i %12.5e %5i  ",
           dc->Number(),
           dc->MatrixElementCode(),
           dc->BranchingRatio(),
           dc->NDaughters());
    
      for (int i=0; i<dc->NDaughters(); i++) {
         int ic = dc->DaughterPdgCode(i);
         TParticlePDG* p = db->GetParticle(ic);
         printf(" %15s(%8i)",p->GetName(),ic);
      }
      printf("\n");
   }
}


//______________________________________________________________________________
void TParticlePDG::Print(Option_t *) const
{
//
//  Print the entire information of this kind of particle
//

   printf("%-20s  %6d\t",GetName(),fPdgCode);
   if (!fStable) {
      printf("Mass:%9.4f Width (GeV):%11.4e\tCharge: %5.1f\n",
              fMass, fWidth, fCharge);
   } else {
      printf("Mass:%9.4f Width (GeV): Stable\tCharge: %5.1f\n",
              fMass, fCharge);
   }
   if (fDecayList) {
      int banner_printed = 0;
      TIter next(fDecayList);
      TDecayChannel* dc;
      while ((dc = (TDecayChannel*)next())) {
         if (! banner_printed) {
            PrintDecayChannel(dc,"banner");
            banner_printed = 1;
         }
         PrintDecayChannel(dc,"data");
      }
   }
}

