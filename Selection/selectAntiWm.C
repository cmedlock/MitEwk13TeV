//================================================================================================
//
// Select W->munu candidates
//
//  * outputs ROOT files of events passing selection
//
//________________________________________________________________________________________________

#if !defined(__CINT__) || defined(__MAKECINT__)
#include <TROOT.h>                  // access to gROOT, entry point to ROOT system
#include <TSystem.h>                // interface to OS
#include <TFile.h>                  // file handle class
#include <TTree.h>                  // class to access ntuples
#include <TClonesArray.h>           // ROOT array class
#include <TBenchmark.h>             // class to track macro running statistics
#include <TVector2.h>               // 2D vector class
#include <TMath.h>                  // ROOT math library
#include <vector>                   // STL vector class
#include <iostream>                 // standard I/O
#include <iomanip>                  // functions to format standard I/O
#include <fstream>                  // functions for file I/O
#include "TLorentzVector.h"     // 4-vector class

#include "ConfParse.hh"             // input conf file parser
#include "../Utils/CSample.hh"      // helper class to handle samples

// define structures to read in ntuple
#include "BaconAna/DataFormats/interface/BaconAnaDefs.hh"
#include "BaconAna/DataFormats/interface/TEventInfo.hh"
#include "BaconAna/DataFormats/interface/TGenEventInfo.hh"
#include "BaconAna/DataFormats/interface/TGenParticle.hh"
#include "BaconAna/DataFormats/interface/TMuon.hh"
#include "BaconAna/DataFormats/interface/TVertex.hh"
#include "BaconAna/Utils/interface/TTrigger.hh"

// lumi section selection with JSON files
//#include "MitAna/DataCont/interface/RunLumiRangeMap.h"

#include "../Utils/LeptonIDCuts.hh" // helper functions for lepton ID selection
#include "../Utils/MyTools.hh"      // various helper functions
#endif

//=== MAIN MACRO ================================================================================================= 

void selectAntiWm(const TString conf="wm.conf", // input file
              const TString outputDir="."       // output directory
) {
  gBenchmark->Start("selectAntiWm");

  //--------------------------------------------------------------------------------------------------------------
  // Settings 
  //============================================================================================================== 

  const Double_t PT_CUT    = 20;
  const Double_t ETA_CUT   = 2.4;
  const Double_t MUON_MASS = 0.105658369;

  const Double_t VETO_PT   = 10;
  const Double_t VETO_ETA  = 2.4;

  const Int_t BOSON_ID  = 24;
  const Int_t LEPTON_ID = 13;

  //--------------------------------------------------------------------------------------------------------------
  // Main analysis code 
  //==============================================================================================================  

  vector<TString>  snamev;      // sample name (for output files)  
  vector<CSample*> samplev;     // data/MC samples

  //
  // parse .conf file
  //
  confParse(conf, snamev, samplev);
  const Bool_t hasData = (samplev[0]->fnamev.size()>0);

  // Create output directory
  gSystem->mkdir(outputDir,kTRUE);
  const TString ntupDir = outputDir + TString("/ntuples");
  gSystem->mkdir(ntupDir,kTRUE);
  
  //
  // Declare output ntuple variables
  //
  UInt_t  runNum, lumiSec, evtNum;
  UInt_t  npv, npu;
  UInt_t  id_1, id_2;
  Double_t x_1, x_2, xPDF_1, xPDF_2;
  Double_t scalePDF, weightPDF;
  TLorentzVector *genV=0, *genLep=0;
  Float_t genVPt, genVPhi, genVy, genVMass;
  Float_t genLepPt, genLepPhi;
  Float_t scale1fb;
  Float_t met, metPhi, sumEt, mt, u1, u2;
  Float_t tkMet, tkMetPhi, tkSumEt, tkMt, tkU1, tkU2;
  Int_t   q;
  TLorentzVector *lep=0;
  ///// muon specific /////
  Float_t trkIso, emIso, hadIso;
  Float_t pfChIso, pfGamIso, pfNeuIso, pfCombIso;
  Float_t d0, dz;
  Float_t muNchi2;
  UInt_t nPixHits, nTkLayers, nValidHits, nMatch, typeBits;
  
  // Data structures to store info from TTrees
  baconhep::TEventInfo *info  = new baconhep::TEventInfo();
  baconhep::TGenEventInfo *gen  = new baconhep::TGenEventInfo();
  TClonesArray *genPartArr = new TClonesArray("baconhep::TGenParticle");
  TClonesArray *muonArr    = new TClonesArray("baconhep::TMuon");
  TClonesArray *pvArr      = new TClonesArray("baconhep::TVertex");
  
  TFile *infile=0;
  TTree *eventTree=0;
  
  //
  // loop over samples
  //  
  for(UInt_t isam=0; isam<samplev.size(); isam++) {
    
    // Assume data sample is first sample in .conf file
    // If sample is empty (i.e. contains no ntuple files), skip to next sample
    if(isam==0 && !hasData) continue;

    // Assume signal sample is given name "wm"                                                                                                      
    Bool_t isSignal = (snamev[isam].CompareTo("wm",TString::kIgnoreCase)==0);
    // flag to reject W->mnu events for wrong flavor backgrounds
    Bool_t isWrongFlavor = (snamev[isam].CompareTo("wx",TString::kIgnoreCase)==0);  
    
    CSample* samp = samplev[isam];
  
    //
    // Set up output ntuple
    //
    TString outfilename = ntupDir + TString("/") + snamev[isam] + TString("_select.root");
    TFile *outFile = new TFile(outfilename,"RECREATE"); 
    TTree *outTree = new TTree("Events","Events");

    outTree->Branch("runNum",     &runNum,     "runNum/i");     // event run number
    outTree->Branch("lumiSec",    &lumiSec,    "lumiSec/i");    // event lumi section
    outTree->Branch("evtNum",     &evtNum,     "evtNum/i");     // event number
    outTree->Branch("npv",        &npv,        "npv/i");        // number of primary vertices
    outTree->Branch("npu",        &npu,        "npu/i");        // number of in-time PU events (MC)
    outTree->Branch("id_1",       &id_1,       "id_1/i");       // PDF info -- parton ID for parton 1
    outTree->Branch("id_2",       &id_2,       "id_2/i");       // PDF info -- parton ID for parton 2
    outTree->Branch("x_1",        &x_1,        "x_1/d");        // PDF info -- x for parton 1
    outTree->Branch("x_2",        &x_2,        "x_2/d");        // PDF info -- x for parton 2
    outTree->Branch("xPDF_1",     &xPDF_1,     "xPDF_1/d");     // PDF info -- x*F for parton 1
    outTree->Branch("xPDF_2",     &xPDF_2,     "xPDF_2/d");     // PDF info -- x*F for parton 2
    outTree->Branch("scalePDF",   &scalePDF,   "scalePDF/d");   // PDF info -- energy scale of parton interaction
    outTree->Branch("weightPDF",  &weightPDF,  "weightPDF/d");  // PDF info -- PDF weight
    outTree->Branch("genV",       "TLorentzVector", &genV);     // GEN boson 4-vector (signal MC)
    outTree->Branch("genLep",     "TLorentzVector", &genLep);   // GEN lepton 4-vector (signal MC)
    outTree->Branch("genVPt",     &genVPt,     "genVPt/F");     // GEN boson pT (signal MC)
    outTree->Branch("genVPhi",    &genVPhi,    "genVPhi/F");    // GEN boson phi (signal MC)
    outTree->Branch("genVy",      &genVy,      "genVy/F");      // GEN boson rapidity (signal MC)
    outTree->Branch("genVMass",   &genVMass,   "genVMass/F");   // GEN boson mass (signal MC)
    outTree->Branch("genLepPt",   &genLepPt, "genLepPt/F");    // GEN lepton pT (signal MC)
    outTree->Branch("genLepPhi",  &genLepPhi,"genLepPhi/F");   // GEN lepton phi (signal MC)
    outTree->Branch("scale1fb",   &scale1fb,   "scale1fb/F");   // event weight per 1/fb (MC)
    outTree->Branch("met",        &met,        "met/F");        // MET
    outTree->Branch("metPhi",     &metPhi,     "metPhi/F");     // phi(MET)
    outTree->Branch("sumEt",      &sumEt,      "sumEt/F");      // Sum ET
    outTree->Branch("mt",         &mt,         "mt/F");         // transverse mass
    outTree->Branch("u1",         &u1,         "u1/F");         // parallel component of recoil
    outTree->Branch("u2",         &u2,         "u2/F");         // perpendicular component of recoil
    outTree->Branch("tkMet",      &tkMet,      "tkMet/F");      // MET (track MET)
    outTree->Branch("tkMetPhi",   &tkMetPhi,   "tkMetPhi/F");   // phi(MET) (track MET)
    outTree->Branch("tkSumEt",    &tkSumEt,    "tkSumEt/F");    // Sum ET (track MET)
    outTree->Branch("tkMt",       &tkMt,       "tkMt/F");       // transverse mass
    outTree->Branch("tkU1",       &tkU1,       "tkU1/F");       // parallel component of recoil (track MET)
    outTree->Branch("tkU2",       &tkU2,       "tkU2/F");       // perpendicular component of recoil (track MET)
    outTree->Branch("q",          &q,          "q/I");          // lepton charge
    outTree->Branch("lep",        "TLorentzVector", &lep);      // lepton 4-vector
    ///// muon specific /////
    outTree->Branch("trkIso",     &trkIso,     "trkIso/F");     // track isolation of lepton
    outTree->Branch("emIso",      &emIso,      "emIso/F");      // ECAL isolation of lepton
    outTree->Branch("hadIso",     &hadIso,     "hadIso/F");     // HCAL isolation of lepton
    outTree->Branch("pfChIso",    &pfChIso,    "pfChIso/F");    // PF charged hadron isolation of lepton
    outTree->Branch("pfGamIso",   &pfGamIso,   "pfGamIso/F");   // PF photon isolation of lepton
    outTree->Branch("pfNeuIso",   &pfNeuIso,   "pfNeuIso/F");   // PF neutral hadron isolation of lepton
    outTree->Branch("pfCombIso",  &pfCombIso,  "pfCombIso/F");  // PF combined isolation of lepton
    outTree->Branch("d0",         &d0,         "d0/F");         // transverse impact parameter of lepton
    outTree->Branch("dz",         &dz,         "dz/F");         // longitudinal impact parameter of lepton
    outTree->Branch("muNchi2",    &muNchi2,    "muNchi2/F");    // muon fit normalized chi^2 of lepton
    outTree->Branch("nPixHits",   &nPixHits,   "nPixHits/i");   // number of pixel hits of muon
    outTree->Branch("nTkLayers",  &nTkLayers,  "nTkLayers/i");  // number of tracker layers of muon
    outTree->Branch("nMatch",     &nMatch,     "nMatch/i");     // number of matched segments of muon	 
    outTree->Branch("nValidHits", &nValidHits, "nValidHits/i"); // number of valid muon hits of muon 
    outTree->Branch("typeBits",   &typeBits,   "typeBits/i");   // number of valid muon hits of muon 
    
    //
    // loop through files
    //
    const UInt_t nfiles = samp->fnamev.size();
    for(UInt_t ifile=0; ifile<nfiles; ifile++) {  

      // Read input file and get the TTrees
      cout << "Processing " << samp->fnamev[ifile] << " [xsec = " << samp->xsecv[ifile] << " pb] ... "; cout.flush();
      infile = TFile::Open(samp->fnamev[ifile]); 
      assert(infile);
      
      //Bool_t hasJSON = kFALSE;
      //baconhep::RunLumiRangeMap rlrm;
      //if(samp->jsonv[ifile].CompareTo("NONE")!=0) { 
      //hasJSON = kTRUE;
      //rlrm.AddJSONFile(samp->jsonv[ifile].Data()); 
      //}

      const baconhep::TTrigger triggerMenu("../../BaconAna/DataFormats/data/HLT_50nsGRun");
      UInt_t trigger    = triggerMenu.getTriggerBit("HLT_IsoMu20_v*");
      UInt_t trigObjL1  = 6;//triggerMenu.getTriggerObjectBit("HLT_IsoMu20_v*", "hltL1sL1SingleMu16");
      UInt_t trigObjHLT = 7;//triggerMenu.getTriggerObjectBit("HLT_IsoMu20_v*",
      //"hltL3crIsoL1sMu16L1f0L2f10QL3f20QL3trkIsoFiltered0p09");

      eventTree = (TTree*)infile->Get("Events");
      assert(eventTree);
      eventTree->SetBranchAddress("Info", &info);    TBranch *infoBr = eventTree->GetBranch("Info");
      eventTree->SetBranchAddress("Muon", &muonArr); TBranch *muonBr = eventTree->GetBranch("Muon");
      Bool_t hasGen = eventTree->GetBranchStatus("GenEvtInfo");
      TBranch *genBr=0, *genPartBr=0;
      if(hasGen) {
        eventTree->SetBranchAddress("GenEvtInfo", &gen);
        genBr = eventTree->GetBranch("GenEvtInfo");
        eventTree->SetBranchAddress("GenParticle",&genPartArr);
        genPartBr = eventTree->GetBranch("GenParticle");
      }
      Bool_t hasVer = eventTree->GetBranchStatus("Vertex");
      TBranch *pvBr=0;
      if (hasVer) {
        eventTree->SetBranchAddress("Vertex", &pvArr); pvBr = eventTree->GetBranch("Vertex");
      }
    
      // Compute MC event weight per 1/fb
      Double_t weight = 1;
      const Double_t xsec = samp->xsecv[ifile];
      if(xsec>0) weight = 1000.*xsec/(Double_t)eventTree->GetEntries();     

      //
      // loop over events
      //
      Double_t nsel=0, nselvar=0;
      for(UInt_t ientry=0; ientry<eventTree->GetEntries(); ientry++) {
        infoBr->GetEntry(ientry);
	
	if(genBr) {
          genBr->GetEntry(ientry);
          genPartArr->Clear();
          genPartBr->GetEntry(ientry);
        }
        
	// check for certified lumi (if applicable)
        //baconhep::RunLumiRangeMap::RunLumiPairType rl(info->runNum, info->lumiSec);      
        //if(hasJSON && !rlrm.HasRunLumi(rl)) continue;  

        // trigger requirement               
        if(!(info->triggerBits[trigger])) continue;
      
        // good vertex requirement
        if(!(info->hasGoodPV)) continue;
	if (hasVer) {
	  pvArr->Clear();
	  pvBr->GetEntry(ientry);
	}

        //
	// SELECTION PROCEDURE:
	//  (1) Look for 1 good muon matched to trigger
	//  (2) Reject event if another muon is present passing looser cuts
	//
	muonArr->Clear();
        muonBr->GetEntry(ientry);
	Int_t nLooseLep=0;
	const baconhep::TMuon *goodMuon=0;
	Bool_t passSel=kFALSE;
        for(Int_t i=0; i<muonArr->GetEntriesFast(); i++) {
          const baconhep::TMuon *mu = (baconhep::TMuon*)((*muonArr)[i]);

          if(fabs(mu->eta) > VETO_PT)  continue; // loose lepton |eta| cut
          if(mu->pt        < VETO_ETA) continue; // loose lepton pT cut
//          if(passMuonLooseID(mu)) nLooseLep++; // loose lepton selection
          if(nLooseLep>1) {  // extra lepton veto
            passSel=kFALSE;
            break;
          }
          
          if(fabs(mu->eta) > ETA_CUT)         continue; // lepton |eta| cut
          if(mu->pt < PT_CUT)                 continue; // lepton pT cut   
          if(!passAntiMuonID(mu))             continue; // lepton anti-selection
          if(!(mu->hltMatchBits[trigObjHLT])) continue; // check trigger matching
	  
	  passSel=kTRUE;
	  goodMuon = mu;

	}

	// veto w -> munu decay for wrong flavor background samples (needed for inclusive WToLNu sample)
        if (isWrongFlavor) {
          TLorentzVector *vec=0, *lep1=0, *lep2=0;
          if (fabs(toolbox::flavor(genPartArr, BOSON_ID, vec, lep1, lep2))==LEPTON_ID) continue;
        }
	
	if(passSel) {	  
	  /******** We have a W candidate! HURRAY! ********/
	  nsel+=weight;
          nselvar+=weight*weight;
	  
	  TLorentzVector vLep; vLep.SetPtEtaPhiM(goodMuon->pt, goodMuon->eta, goodMuon->phi, MUON_MASS);
	  //
	  // Fill tree
	  //
	  runNum   = info->runNum;
	  lumiSec  = info->lumiSec;
	  evtNum   = info->evtNum;
	  npv	   = hasVer ? pvArr->GetEntriesFast() : 0;
	  npu	   = info->nPU;
	  genV      = new TLorentzVector(0,0,0,0);
          genLep    = new TLorentzVector(0,0,0,0);
	  genVPt    = -999;
          genVPhi   = -999;
          genVy     = -999;
          genVMass  = -999;
          u1        = -999;
          u2        = -999;
          tkU1      = -999;
          tkU2      = -999;
          id_1      = -999;
          id_2      = -999;
          x_1       = -999;
          x_2       = -999;
          xPDF_1    = -999;
          xPDF_2    = -999;
          scalePDF  = -999;
          weightPDF = -999;
	  if(isSignal) {
	    TLorentzVector *vec=0, *lep1=0, *lep2=0;
	    // veto wrong flavor events for signal sample
            if (fabs(toolbox::flavor(genPartArr, BOSON_ID, vec, lep1, lep2))!=LEPTON_ID) continue;
            if (vec && lep1) {
	      genV      = new TLorentzVector(0,0,0,0);
              genV->SetPtEtaPhiM(vec->Pt(),vec->Eta(),vec->Phi(),vec->M());
              genLep    = new TLorentzVector(0,0,0,0);
              genLep->SetPtEtaPhiM(lep1->Pt(),lep1->Eta(),lep1->Phi(),lep1->M());
              genVPt    = vec->Pt();
              genVPhi   = vec->Phi();
              genVy     = vec->Rapidity();
              genVMass  = vec->M();
              genLepPt  = lep1->Pt();
              genLepPhi = lep1->Phi();

	      TVector2 vWPt((genVPt)*cos(genVPhi),(genVPt)*sin(genVPhi));
              TVector2 vLepPt(vLep.Px(),vLep.Py());

              TVector2 vMet((info->pfMET)*cos(info->pfMETphi), (info->pfMET)*sin(info->pfMETphi));
              TVector2 vU = -1.0*(vMet+vLepPt);
              u1 = ((vWPt.Px())*(vU.Px()) + (vWPt.Py())*(vU.Py()))/(genVPt);  // u1 = (pT . u)/|pT|
              u2 = ((vWPt.Px())*(vU.Py()) - (vWPt.Py())*(vU.Px()))/(genVPt);  // u2 = (pT x u)/|pT|

              TVector2 vTkMet((info->trkMET)*cos(info->trkMETphi), (info->trkMET)*sin(info->trkMETphi));
              TVector2 vTkU = -1.0*(vTkMet+vLepPt);
              tkU1 = ((vWPt.Px())*(vTkU.Px()) + (vWPt.Py())*(vTkU.Py()))/(genVPt);  // u1 = (pT . u)/|pT|
              tkU2 = ((vWPt.Px())*(vTkU.Py()) - (vWPt.Py())*(vTkU.Px()))/(genVPt);  // u2 = (pT x u)/|pT|
            }
	    id_1      = gen->id_1;
            id_2      = gen->id_2;
            x_1       = gen->x_1;
            x_2       = gen->x_2;
            xPDF_1    = gen->xPDF_1;
            xPDF_2    = gen->xPDF_2;
            scalePDF  = gen->scalePDF;
            weightPDF = gen->weight;
	  }
	  scale1fb = weight;
	  met	   = info->pfMET;
	  metPhi   = info->pfMETphi;
	  sumEt    = 0;
	  mt       = sqrt( 2.0 * (vLep.Pt()) * (info->pfMET) * (1.0-cos(toolbox::deltaPhi(vLep.Phi(),info->pfMETphi))) );
	  tkMet    = info->trkMET;
          tkMetPhi = info->trkMETphi;
          tkSumEt  = 0;
          tkMt     = sqrt( 2.0 * (vLep.Pt()) * (info->trkMET) * (1.0-cos(toolbox::deltaPhi(vLep.Phi(),info->trkMETphi))) );
	  q        = goodMuon->q;
	  lep      = &vLep;

	  ///// muon specific /////
	  trkIso     = goodMuon->trkIso;
          emIso      = goodMuon->ecalIso;
          hadIso     = goodMuon->hcalIso;
          pfChIso    = goodMuon->chHadIso;
          pfGamIso   = goodMuon->gammaIso;
          pfNeuIso   = goodMuon->neuHadIso;
          pfCombIso  = goodMuon->chHadIso + TMath::Max(goodMuon->neuHadIso + goodMuon->gammaIso -
						       0.5*(goodMuon->puIso),Double_t(0));
	  d0         = goodMuon->d0;
	  dz         = goodMuon->dz;
	  muNchi2    = goodMuon->muNchi2;
	  nPixHits   = goodMuon->nPixHits;
	  nTkLayers  = goodMuon->nTkLayers;
	  nMatch     = goodMuon->nMatchStn;
	  nValidHits = goodMuon->nValidHits;
	  typeBits   = goodMuon->typeBits;

	  outTree->Fill();
	  genV=0, genLep=0, lep=0;
        }
      }
      delete infile;
      infile=0, eventTree=0;    

      cout << nsel  << " +/- " << sqrt(nselvar);
      if(isam!=0) cout << " per 1/fb";
      cout << endl;
    }
    outFile->Write();
    outFile->Close();
  }
  delete info;
  delete gen;
  delete muonArr;
  delete pvArr;
  
    
  //--------------------------------------------------------------------------------------------------------------
  // Output
  //==============================================================================================================
   
  cout << "*" << endl;
  cout << "* SUMMARY" << endl;
  cout << "*--------------------------------------------------" << endl;
  cout << " W -> mu nu" << endl;
  cout << "  pT > " << PT_CUT << endl;
  cout << "  |eta| < " << ETA_CUT << endl;
  cout << endl;
  
  cout << endl;
  cout << "  <> Output saved in " << outputDir << "/" << endl;    
  cout << endl;  
      
  gBenchmark->Show("selectAntiWm"); 
}
