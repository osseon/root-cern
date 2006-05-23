// @(#)root/base:$Name: v5-11-02 $:$Id: TContextMenu.cxx,v 1.13 2006/03/20 21:43:41 pcanal Exp $
// Author: Nenad Buncic   08/02/96

/*************************************************************************
 * Copyright (C) 1995-2000, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// TContextMenu                                                         //
//                                                                      //
// This class provides an interface to  context sensitive popup menus.  //
// These menus pop up when the user hits the right mouse button, and    //
// are destroyed when the menu pops downs.                              //
//                                                                      //
// Context Menus are automatically generated by ROOT using the          //
// following convention: if the string // *MENU* is found in the        //
// comment field of a member function. This function will be added to   //
// the list of items in the menu.                                       //
//                                                                      //
// The picture below shows a canvas with a pop-up menu.                 //
//                                                                      //
//Begin_Html <img src="gif/hsumMenu.gif"> End_Html                      //
//                                                                      //
// The picture below shows a canvas with a pop-up menu and a dialog box.//
//                                                                      //
//Begin_Html <img src="gif/hsumDialog.gif"> End_Html                    //
//////////////////////////////////////////////////////////////////////////

#include "TROOT.h"
#include "TContextMenu.h"
#include "TVirtualPad.h"
#include "TGuiFactory.h"
#include "TMethod.h"
#include "TMethodArg.h"
#include "TGlobal.h"
#include "TObjArray.h"
#include "TObjString.h"
#include "TToggle.h"
#include "TClassMenuItem.h"
#include "TBrowser.h"
#include "TClass.h"

ClassImp(TContextMenu)


//______________________________________________________________________________
TContextMenu::TContextMenu(const char *name, const char *title)
             : TNamed(name, title)
{
   // Create a context menu.

   fSelectedObject   = 0;
   fSelectedMethod   = 0;
   fBrowser          = 0;
   fSelectedPad      = 0;
   fSelectedCanvas   = 0;
   fSelectedMenuItem = 0;

   fContextMenuImp = gGuiFactory->CreateContextMenuImp(this, name, title);
}

//______________________________________________________________________________
TContextMenu::TContextMenu(const TContextMenu& cm) :
  TNamed(cm),
  fContextMenuImp(cm.fContextMenuImp),
  fSelectedMethod(cm.fSelectedMethod),
  fSelectedObject(cm.fSelectedObject),
  fCalledObject(cm.fCalledObject),
  fSelectedMenuItem(cm.fSelectedMenuItem),
  fSelectedCanvas(cm.fSelectedCanvas),
  fSelectedPad(cm.fSelectedPad),
  fBrowser(cm.fBrowser)
{ }

//______________________________________________________________________________
TContextMenu& TContextMenu::operator=(const TContextMenu& cm)
{
  if(this!=&cm) {
    TNamed::operator=(cm);
    fContextMenuImp=cm.fContextMenuImp;
    fSelectedMethod=cm.fSelectedMethod;
    fSelectedObject=cm.fSelectedObject;
    fCalledObject=cm.fCalledObject;
    fSelectedMenuItem=cm.fSelectedMenuItem;
    fSelectedCanvas=cm.fSelectedCanvas;
    fSelectedPad=cm.fSelectedPad;
    fBrowser=cm.fBrowser;
  } return *this;
}

//______________________________________________________________________________
TContextMenu::~TContextMenu()
{
   // Destroy a context menu.

   delete fContextMenuImp;

   fSelectedMethod   = 0;
   fSelectedObject   = 0;
   fSelectedMenuItem = 0;
   fContextMenuImp   = 0;
}

//______________________________________________________________________________
void TContextMenu::Action(TObject *object, TMethod *method)
{
   // Action to be performed when this menu item is selected.
   // If the selected method requires arguments we popup an
   // automatically generated dialog, otherwise the method is
   // directly executed.

   if (method) {
      SetMethod( method );
      SetSelectedMenuItem(0);
      SetCalledObject(object);

      if (method->GetListOfMethodArgs()->First())
         fContextMenuImp->Dialog(object, method);
      else {
         Execute(object, method, "");
      }
   }

   if (fBrowser) fBrowser->Refresh();
}

//______________________________________________________________________________
void TContextMenu::Action(TClassMenuItem *menuitem)
{
   // Action to be performed when this menu item is selected.
   // If the selected method requires arguments we popup an
   // automatically generated dialog, otherwise the method is
   // directly executed.

   TObject* object;
   TMethod* method = 0;

   SetSelectedMenuItem( menuitem );

   // Get the object to be called
   if (menuitem->IsCallSelf()) object=fSelectedObject;
   else object=menuitem->GetCalledObject();

   if (object) {
      // If object deleted, remove from popup and return
      if (!(object->TestBit(kNotDeleted))) {
         menuitem->SetType(TClassMenuItem::kPopupSeparator);
         menuitem->SetCall(0,"");
         return;
      }

      method = object->IsA()->GetMethodWithPrototype(menuitem->GetFunctionName(),menuitem->GetArgs());

   }
//    if (!menuitem->IsCallSelf()) {
//       funproto = menuitem->GetFunctionName();
//       funproto = funproto + "(" + menuitem->GetArgs() + ")";
//    }

   // calling object, call the method directly
   if (object) {
      if (method) {
         SetMethod(method);
         SetCalledObject(object);

         if ((method->GetListOfMethodArgs()->First()
                            && menuitem->GetSelfObjectPos() < 0 ) ||
              method->GetListOfMethodArgs()->GetSize() > 1)
            fContextMenuImp->Dialog(object, method);
         else {
            if (menuitem->GetSelfObjectPos() < 0) {
#ifndef WIN32
               Execute(object, method, "");
#else
               // It is a workaround of the "Dead lock under Windows
               char *cmd = Form("((TContextMenu *)0x%lx)->Execute((TObject *)0x%lx,"
                                "(TMethod *)0x%lx,(TObjArray *)0);",
                                (Long_t)this,(Long_t)object,(Long_t)method);
               //Printf("%s", cmd);
               gROOT->ProcessLine(cmd);
               //Execute( object, method, (TObjArray *)NULL );
#endif
            } else {
#ifndef WIN32
               Execute(object, method, Form("(TObject*)0x%lx",(Long_t)fSelectedObject));
#else
               // It is a workaround of the "Dead lock under Windows
               char *cmd = Form("((TContextMenu *)0x%lx)->Execute((TObject *)0x%lx,"
                                "(TMethod *)0x%lx,(TObjArray *)0);",
                                (Long_t)this,(Long_t)object,(Long_t)method);
               //Printf("%s", cmd);
               gROOT->ProcessLine(cmd);
               //Execute( object, method, (TObjArray *)NULL );
#endif
            }
         }
      }

   } else {
      // Calling a standalone global function
      TFunction* function = gROOT->GetGlobalFunctionWithPrototype(
                                 menuitem->GetFunctionName());
                                 //menuitem->GetArgs());
      if (function) {
         SetMethod(function);
         SetCalledObject(0);
         if ( (function->GetNargs() && menuitem->GetSelfObjectPos() < 0) ||
               function->GetNargs() > 1) {
            fContextMenuImp->Dialog(0,function);
         } else {
            char* cmd;
            if (menuitem->GetSelfObjectPos() < 0) {
               cmd = Form("%s();", menuitem->GetFunctionName());
            } else {
              cmd = Form("%s((TObject*)0x%lx);",
                     menuitem->GetFunctionName(), (Long_t)fSelectedObject);
            }
            gROOT->ProcessLine(cmd);
         }
      }
   }

   if (fBrowser) fBrowser->Refresh();
}

//______________________________________________________________________________
void TContextMenu::Action(TObject *object, TToggle *toggle)
{
   // Action to be performed when this toggle menu item is selected.

   if (object && toggle) {
      TVirtualPad *savedPad = 0;

      gROOT->SetSelectedPrimitive(object);
      if (fSelectedPad) {
         savedPad = (TVirtualPad *) gPad;
         if (savedPad) fSelectedPad->cd();
      }

      gROOT->SetFromPopUp(kTRUE);
      toggle->Toggle();
      if (fSelectedCanvas && fSelectedCanvas->GetPadSave()->TestBit(kNotDeleted))
         fSelectedCanvas->GetPadSave()->Modified();
      if (fSelectedPad && fSelectedPad->TestBit(kNotDeleted))
         fSelectedPad->Modified();
      gROOT->SetFromPopUp(kFALSE);

      if (fSelectedPad && savedPad) {
         if (savedPad->TestBit(kNotDeleted)) savedPad->cd();
      }

      if (fSelectedCanvas && fSelectedCanvas->TestBit(kNotDeleted))
         fSelectedCanvas->Update();
      if (fSelectedCanvas && fSelectedCanvas->GetPadSave()->TestBit(kNotDeleted))
         fSelectedCanvas->GetPadSave()->Update();
   }

   if (fBrowser) fBrowser->Refresh();
}

//______________________________________________________________________________
char *TContextMenu::CreateArgumentTitle(TMethodArg *argument)
{
   // Create string describing argument (for use in dialog box).

   static char argTitle[128];

   if (argument) {
      sprintf(argTitle, "(%s)  %s", argument->GetTitle(), argument->GetName());
      if (argument->GetDefault() && *(argument->GetDefault())) {
         strcat(argTitle, "  [default: ");
         strcat(argTitle, argument->GetDefault());
         strcat(argTitle, "]");
      }
   } else
      *argTitle = 0;

   return argTitle;
}

//______________________________________________________________________________
char *TContextMenu::CreateDialogTitle(TObject *object, TFunction *method)
{
   // Create title for dialog box retrieving argument values.

   static char methodTitle[128];

   if (object && method)
      sprintf(methodTitle, "%s::%s", object->ClassName(), method->GetName());
   else if (!object && method)
      sprintf(methodTitle, "%s", method->GetName());
   else
      *methodTitle = 0;

   return methodTitle;
}

//______________________________________________________________________________
char *TContextMenu::CreatePopupTitle(TObject *object)
{
   // Create title for popup menu.

   static char popupTitle[128];

   if (object) {
      if (!*(object->GetName()) || !strcmp(object->GetName(), object->ClassName())) {
         TGlobal *global = (TGlobal *) gROOT->GetGlobal(object);
         if (global && *(global->GetName()))
            sprintf(popupTitle, "  %s::%s  ", object->ClassName(), global->GetName());
         else {
            if (!strcmp(object->IsA()->GetContextMenuTitle(), ""))
               sprintf(popupTitle, "  %s  ", object->ClassName());
            else
               sprintf(popupTitle, "  %s  ", object->IsA()->GetContextMenuTitle());
         }
      } else {
         if (!strcmp(object->IsA()->GetContextMenuTitle(), ""))
            sprintf(popupTitle, "  %s::%s  ", object->ClassName(), object->GetName());
         else
            sprintf(popupTitle, "  %s::%s  ", object->IsA()->GetContextMenuTitle(),
                    object->GetName());
      }
   } else
      *popupTitle = 0;

   return popupTitle;
}

//______________________________________________________________________________
void TContextMenu::Execute(TObject *object, TFunction *method, const char *params)
{
   // Execute method with specified arguments for specified object.

   if (method) {
      TVirtualPad *savedPad = 0;

      gROOT->SetSelectedPrimitive(object);
      if (fSelectedPad) {
         savedPad = (TVirtualPad *) gPad;
         if (savedPad) fSelectedPad->cd();
      }

      gROOT->SetFromPopUp(kTRUE);
//      if (fSelectedCanvas) fSelectedCanvas->GetPadSave()->cd();
      if (object) {
         object->Execute((char *) method->GetName(), params);
      } else {
         char *cmd = Form("%s(%s);", method->GetName(),params);
         gROOT->ProcessLine(cmd);
      }
      if (fSelectedCanvas && fSelectedCanvas->GetPadSave()->TestBit(kNotDeleted))
         fSelectedCanvas->GetPadSave()->Modified();
      if (fSelectedPad && fSelectedPad->TestBit(kNotDeleted))
         fSelectedPad->Modified();
      gROOT->SetFromPopUp( kFALSE );

      if (fSelectedPad && savedPad) {
//         fSelectedPad->Modified();
         if (savedPad->TestBit(kNotDeleted)) savedPad->cd();
      }

      if (fSelectedCanvas && fSelectedCanvas->TestBit(kNotDeleted))
         fSelectedCanvas->Update();
      if (fSelectedCanvas && fSelectedCanvas->GetPadSave()->TestBit(kNotDeleted))
         fSelectedCanvas->GetPadSave()->Update();
   }

   if (fBrowser) fBrowser->Refresh();
}

//______________________________________________________________________________
void TContextMenu::Execute(TObject *object, TFunction *method, TObjArray *params)
{
   // Execute method with specified arguments for specified object.

   if (method) {
      TVirtualPad *savedPad = 0;

      gROOT->SetSelectedPrimitive(object);
      if (fSelectedPad) {
         savedPad = (TVirtualPad *) gPad;
         if (savedPad) fSelectedPad->cd();
      }

      gROOT->SetFromPopUp(kTRUE);
//      if (fSelectedCanvas) fSelectedCanvas->GetPadSave()->cd();
      if (object) {
         object->Execute((TMethod*)method, params);
      } else {
         TString args;
         TIter next(params);
         TObjString *s;
         while ((s = (TObjString*) next())) {
            if (!args.IsNull()) args += ",";
            args += s->String();
         }
         char *cmd = Form("%s(%s);", method->GetName(), args.Data());
         gROOT->ProcessLine(cmd);
      }
      if (fSelectedCanvas && fSelectedCanvas->GetPadSave()->TestBit(kNotDeleted))
         fSelectedCanvas->GetPadSave()->Modified();
      gROOT->SetFromPopUp( kFALSE );

      if (fSelectedPad && savedPad) {
//         fSelectedPad->Modified();
         if (savedPad->TestBit(kNotDeleted)) savedPad->cd();
      }

      if (fSelectedCanvas && fSelectedCanvas->TestBit(kNotDeleted))
         fSelectedCanvas->Update();
   }
   if (fBrowser) fBrowser->Refresh();
}

//______________________________________________________________________________
void TContextMenu::Popup(Int_t x, Int_t y, TObject *obj, TVirtualPad *c, TVirtualPad *p)
{
   // Popup context menu at given location in canvas c and pad p for selected
   // object.

   SetBrowser(0);
   SetObject(obj);
   SetCanvas(c);
   SetPad(p);

   DisplayPopUp(x,y);
}

//______________________________________________________________________________
void TContextMenu::Popup(Int_t x, Int_t y, TObject *obj, TBrowser *b)
{
   // Popup context menu at given location in browser b for selected object.

   SetBrowser(b);
   SetObject(obj);
   SetCanvas(0);
   SetPad(0);

   DisplayPopUp(x,y);
}
