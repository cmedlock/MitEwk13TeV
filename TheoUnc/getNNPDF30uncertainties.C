#if !defined(__CINT__) || defined(__MAKECINT__)
#include <TROOT.h>                  // access to gROOT, entry point to ROOT system
#include <TSystem.h>                // interface to OS
#include <TFile.h>                  // file handle class
#include <TTree.h>                  // class to access ntuples
#include <TClonesArray.h>           // ROOT array class
#include <TBenchmark.h>             // class to track macro running statistics
#include <TVector2.h>               // 2D vector class
#include <TMath.h>                  // ROOT math library
#include <TRandom3.h>
#include <vector>                   // STL vector class
#include <iostream>                 // standard I/O
#include <sstream>                 // standard I/O
#include <iomanip>                  // functions to format standard I/O
#include <fstream>                  // functions for file I/O
#include <TChain.h>
#include <TH1.h>
#include "Math/LorentzVector.h"     // 4-vector class

#include "../Utils/MitStyleRemix.hh"

using namespace std;

#endif

enum PROC1 {wme=0, wpe, wmm, wpm};
enum PROC2 {zee=0, zmm};

void calcAcceptance(TFile *f, TString chan, Int_t as, Int_t n, vector<Double_t> &scale);
void makeScaleVector(TString inDir, TString chan, Double_t &nom, vector<Double_t> &vScales);
Double_t uncert(vector<Double_t> &vScale, Double_t nom);

void getNNPDF30uncertainties() {

  Double_t wmXsec = 8.35;
  Double_t wpXsec = 11.20;
  Double_t zXsec  = 1.92;

  TString inDir = "/afs/cern.ch/work/j/jlawhorn/public/wz-13tev-envelopes-new/";

  cout << "NNPDF3.0 acceptances" << endl;

  Double_t wmeNom; vector<Double_t> wmeScales;
  makeScaleVector(inDir, "wme", wmeNom, wmeScales);

  Double_t wpeNom; vector<Double_t> wpeScales;
  makeScaleVector(inDir, "wpe", wpeNom, wpeScales);

  Double_t wmmNom; vector<Double_t> wmmScales;
  makeScaleVector(inDir, "wmm", wmmNom, wmmScales);

  Double_t wpmNom; vector<Double_t> wpmScales;
  makeScaleVector(inDir, "wpm", wpmNom, wpmScales);

  Double_t zeeNom; vector<Double_t> zeeScales;
  makeScaleVector(inDir, "zee", zeeNom, zeeScales);

  Double_t zmmNom; vector<Double_t> zmmScales;
  makeScaleVector(inDir, "zmm", zmmNom, zmmScales);

  if (wmeScales.size()!=wpeScales.size()) cout << "SOMETHING BAD HAPPENED TO FILES" << endl;
  if (wmeScales.size()!=wpmScales.size()) cout << "SOMETHING BAD HAPPENED TO FILES" << endl;
  if (wmmScales.size()!=wpmScales.size()) cout << "SOMETHING BAD HAPPENED TO FILES" << endl;
  if (zmmScales.size()!=wpmScales.size()) cout << "SOMETHING BAD HAPPENED TO FILES" << endl;
  if (zmmScales.size()!=zeeScales.size()) cout << "SOMETHING BAD HAPPENED TO FILES" << endl;

  Double_t wpmeNom=wpeNom/wmeNom; 
  vector<Double_t> wpmeScales;
  for (UInt_t i=0; i<wmeScales.size(); i++) { wpmeScales.push_back(wpeScales[i]/wmeScales[i]); }

  Double_t wpmmNom=wpmNom/wmmNom; 
  vector<Double_t> wpmmScales;
  for (UInt_t i=0; i<wmmScales.size(); i++) { wpmmScales.push_back(wpmScales[i]/wmmScales[i]); }

  Double_t weNom=(wpeNom*wpXsec+wmeNom*wmXsec)/(wpXsec+wmXsec); 
  vector<Double_t> weScales;
  for (UInt_t i=0; i<wpeScales.size(); i++) { weScales.push_back((wpeScales[i]*wpXsec+wmeScales[i]*wmXsec)/(wpXsec+wmXsec)); }

  Double_t wmNom=(wpmNom*wpXsec+wmmNom*wmXsec)/(wpXsec+wmXsec); 
  vector<Double_t> wmScales;
  for (UInt_t i=0; i<wpmScales.size(); i++) { wmScales.push_back((wpmScales[i]*wpXsec+wmmScales[i]*wmXsec)/(wpXsec+wmXsec)); }

  Double_t wzeNom=(wpeNom*wpXsec+wmeNom*wmXsec)/(zeeNom*zXsec)*((zXsec)/(wpXsec+wmXsec)); 
  vector<Double_t> wzeScales;
  for (UInt_t i=0; i<wpeScales.size(); i++) { wzeScales.push_back(((wpeScales[i]*wpXsec+wmeScales[i]*wmXsec)/(zeeScales[i]*zXsec)*((zXsec)/(wpXsec+wmXsec)))); }

  Double_t wzmNom=(wpmNom*wpXsec+wmmNom*wmXsec)/(zmmNom*zXsec)*((zXsec)/(wpXsec+wmXsec)); 
  vector<Double_t> wzmScales;
  for (UInt_t i=0; i<wpmScales.size(); i++) { wzmScales.push_back(((wpmScales[i]*wpXsec+wmmScales[i]*wmXsec)/(zmmScales[i]*zXsec)*((zXsec)/(wpXsec+wmXsec)))); }

  cout << "uncertainties" << endl;
  cout << "W+m:      " << uncert(wpmScales, wpmNom) << " %" << endl;
  cout << "W-m:      " << uncert(wmmScales, wmmNom) << " %" << endl;
  cout << "W+e:      " << uncert(wpeScales, wpeNom) << " %" << endl;
  cout << "W-e:      " << uncert(wmeScales, wmeNom) << " %" << endl;
  cout << "Zmm:      " << uncert(zmmScales, zmmNom) << " %" << endl;
  cout << "Zee:      " << uncert(zeeScales, zeeNom) << " %" << endl;

  cout << "W+/W-(m): " << uncert(wpmmScales, wpmmNom) << " %" << endl;
  cout << "W+/W-(e): " << uncert(wpmeScales, wpmeNom) << " %" << endl;
  cout << "W(m):     " << uncert(wmScales, wmNom) << " %" << endl;
  cout << "W(e):     " << uncert(weScales, weNom) << " %" << endl;
  cout << "W/Z(m):   " << uncert(wzmScales, wzmNom) << " %" << endl;
  cout << "W/Z(e):   " << uncert(wzeScales, wzeNom) << " %" << endl;

}
void makeScaleVector(TString inDir, TString chan, Double_t &nom, vector<Double_t> &vScales) {

  //cout << chan << endl;

  TString inFile = inDir+chan+"_NNPDF30_nlo_as_0115.root";

  TFile *f = new TFile(inFile, "read");
  vector<Double_t> scale0115;
  calcAcceptance(f, chan, 15, 5, scale0115);
  delete f;

  inFile = inDir+chan+"_NNPDF30_nlo_as_0117.root";
  f = new TFile(inFile, "read");
  vector<Double_t> scale0117;
  calcAcceptance(f, chan, 17, 72, scale0117);
  delete f;

  inFile = inDir+chan+"_NNPDF30_nlo_as_0118.root";
  f = new TFile(inFile, "read");
  vector<Double_t> scale0118;
  calcAcceptance(f, chan, 18, 100, scale0118);
  delete f;

  inFile = inDir+chan+"_NNPDF30_nlo_as_0119.root";
  f = new TFile(inFile, "read");
  vector<Double_t> scale0119;
  calcAcceptance(f, chan, 19, 72, scale0119);
  delete f;

  inFile = inDir+chan+"_NNPDF30_nlo_as_0121.root";
  f = new TFile(inFile, "read");
  vector<Double_t> scale0121;
  calcAcceptance(f, chan, 21, 5, scale0121);
  delete f;

  nom=scale0118[0];

  for (UInt_t i=0; i<scale0115.size(); i++) { vScales.push_back(scale0115[i]); }
  for (UInt_t i=0; i<scale0117.size(); i++) { vScales.push_back(scale0117[i]); }
  for (UInt_t i=1; i<scale0118.size(); i++) { vScales.push_back(scale0118[i]); }
  for (UInt_t i=0; i<scale0119.size(); i++) { vScales.push_back(scale0119[i]); }
  for (UInt_t i=0; i<scale0121.size(); i++) { vScales.push_back(scale0121[i]); }

}
void calcAcceptance(TFile *f, TString chan, Int_t as, Int_t n, vector<Double_t> &scale) {
  char hname[100];
  
  for (Int_t i=0; i<n; i++) {

    Bool_t prnt=kFALSE;
    if (as==18 && i==0) prnt=kTRUE; 
    
    if (prnt) cout << chan << " ";
    //if (prnt) cout << "NNPDF3.0nlo a_s = 0.1" << as << ", i = " << i << endl;

    sprintf(hname, "dTot_NNPDF30_nlo_as_01%i_%i", as, i);
    TH1D *tot = (TH1D*) f->Get(hname);
    Double_t s = tot->Integral()/tot->GetEntries();

    Double_t accept;
    if (chan=="wmm" || chan=="wme" || chan=="wpe" || chan=="wpm") {
      sprintf(hname, "dPostB_NNPDF30_nlo_as_01%i_%i", as, i);
      TH1D *bar = (TH1D*) f->Get(hname);
      accept=bar->Integral()/(tot->Integral());
      //if (prnt) cout << "Barrel Acceptance: " << accept << " +/- " << sqrt(accept*(1-accept)/tot->GetEntries()) << endl;

      sprintf(hname, "dPostE_NNPDF30_nlo_as_01%i_%i", as, i);
      TH1D *end = (TH1D*) f->Get(hname);
      accept=end->Integral()/(tot->Integral());
      //if (prnt) cout << "Endcap Acceptance: " << accept << " +/- " << sqrt(accept*(1-accept)/tot->GetEntries()) << endl;

      accept=(bar->Integral()+end->Integral())/(tot->Integral());
    }
    else if (chan=="zmm" || chan=="zee") {
      sprintf(hname, "dPostBB_NNPDF30_nlo_as_01%i_%i", as, i);
      TH1D *bb = (TH1D*) f->Get(hname);
      accept=bb->Integral()/(tot->Integral());
      //if (prnt) cout << "Bar-bar Acceptance: " << accept << " +/- " << sqrt(accept*(1-accept)/tot->GetEntries()) << endl;

      sprintf(hname, "dPostBE_NNPDF30_nlo_as_01%i_%i", as, i);
      TH1D *be = (TH1D*) f->Get(hname);
      accept=be->Integral()/(tot->Integral());
      //if (prnt) cout << "Bar-end Acceptance: " << accept << " +/- " << sqrt(accept*(1-accept)/tot->GetEntries()) << endl;

      sprintf(hname, "dPostEE_NNPDF30_nlo_as_01%i_%i", as, i);
      TH1D *ee = (TH1D*) f->Get(hname);
      accept=ee->Integral()/(tot->Integral());
      //if (prnt) cout << "End-end Acceptance: " << accept << " +/- " << sqrt(accept*(1-accept)/tot->GetEntries()) << endl;

      accept=(bb->Integral()+be->Integral()+ee->Integral())/(tot->Integral());
    }
    if (prnt) cout << accept << " +/- " << sqrt(accept*(1-accept)/tot->GetEntries()) << endl;

    //if (prnt) cout << "Scale: " << tot->Integral()/tot->GetEntries() << endl;

    //scale.push_back(s);
    scale.push_back(accept);
  }
}

Double_t uncert(vector<Double_t> &vScale, Double_t nom) {

  Double_t stdU=0, stdL=0;
  Int_t nP=0, nM=0;

  for (UInt_t i=1; i<vScale.size(); i++) {
    if ((vScale[i]-nom)>0) {
      stdU+=(vScale[i]-nom)*(vScale[i]-nom);
      nP++;
    }
    else if ((vScale[i]-nom)<0) {
      stdL+=(vScale[i]-nom)*(vScale[i]-nom);
      nM++;
    }
  }

  stdU=sqrt(stdU/(nP-1));
  stdL=sqrt(stdL/(nM-1));

  return max(stdU/nom*100, stdL/nom*100);

}
