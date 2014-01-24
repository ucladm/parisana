/*
  Analysis code for event reconstruction in UCLA EXACT TPCs.

  Two parts to code:
  1. Event by event processing, including baseline finding, pulse finding, etc
  2. Apply cuts and generate histograms and average waveforms
  
  Compile with Makefile
  
  v0.1 AFan 2013-04-21


  2013-04-21
  implement multiple file processing later

 */


#include <iostream>
#include <string>
#include <fstream>


#include "TROOT.h"

#include "TApplication.h"
#include "TCanvas.h"
#include "TH1F.h"
#include "TFile.h"
#include "TTree.h"
#include "TBranch.h"

#include "DAQheader.hh"
#include "CfgReader.hh"
#include "EventData.hh"
#include "Converter.hh"
#include "BaselineFinder.hh"
#include "Integrator.hh"

#include <string>

using namespace std;


int ProcessEvents(DAQheader& DAQ_header, string cfgFile )
{

    
  // Instantiate CfgReader
  CfgReader cfg;
  if (!cfg.readCfgFile(cfgFile)) {
    std::cout << "ERROR reading cfg file" << std::endl;
    return 0;
  }

  // Create TTree to hold all processd info and open a file
  // to hold the TTree.
  TTree* tree = new TTree("Events", "EventData");
  TFile* rootfile = new TFile("output.root", "RECREATE");


  // Instantiate EventData; will repopulate this object for each
  // event. Create a branch to hold all event data.
  EventData* event = new EventData();
  //tree->Branch("EventData", "EventData", &event);
  tree->Branch("run_id", &(event->run_id));
  tree->Branch("event_id", &(event->event_id));
  tree->Branch("nchans", &(event->nchans));
  tree->Branch("nsamps", &(event->nsamps));
  tree->Branch("us_per_samp", &(event->us_per_samp));
  tree->Branch("trigger_index", &(event->trigger_index));
  tree->Branch("channel_id", &(event->channel_id));
  tree->Branch("spe_mean", &(event->spe_mean));
  tree->Branch("baseline_mean", &(event->baseline_mean));
  tree->Branch("baseline_sigma", &(event->baseline_sigma));
  tree->Branch("baseline_valid", &(event->baseline_valid));
  
  
  // ------------------ INSTANTIATE ALL MODULES -------------------
  Converter converter(cfg);
  BaselineFinder baselineFinder(cfg);
  Integrator integrator(cfg);

  //---------------- INITIALIZE MODULES (AS NEEDED) ---------------

  
  // -------------------- LOOP OVER EVENTS ------------------------
  int min_evt = cfg.getParam<int>("tpcanalysis", "min", 1);
  int max_evt = cfg.getParam<int>("tpcanalysis", "max",
                                  DAQ_header.TotEventNbr);
  std::cout << "\nBeginning loop over events.\n" << std::endl;
  for (int evt=min_evt; evt<=max_evt; evt++) {
    if (evt%10000 == 0)
      std::cout << "Processing event " << evt << std::endl;
    event->Clear();
    event->run_id = 0;
    event->event_id = evt;
    
    // Run processing modules on event
    converter.Process(event, DAQ_header); 
    baselineFinder.Process(event);
    integrator.Process(event);

    
    tree->Fill();
    

  }// end loop over events

  //----------------- FINALIZE MODULES (AS NEEDED) ---------------


  // write TTree to file
  tree->Write();
  rootfile->Close();

  return 1;
}


//----------------------------------------------------------------
//----------------------------------------------------------------
//----------------------------------------------------------------


int main(int args, char* argv[]) {

  if (args!=3) {
    std::cout << "Use correct command: ./tpcanalysis <cfg file> <rawdatafile>"
              << std::endl;
    return 1;
  }
  
  // initialize ROOT application
  TApplication *theApp = new TApplication("app", 0, 0);
  theApp->SetReturnFromRun(false);

  // initialize DAQheader
  DAQheader DAQ_header;
  if (DAQ_header.FormatTest()==false)
    std::cout << "ALARM! Variable size doesn't match!" << std::endl;

  // open raw data file
  string datafile = argv[2];
  DAQ_header.LoadFileName(datafile.c_str());
  if (!DAQ_header.binary_file.is_open()) {
    std::cout << std::endl << "Can't open datafile: "
              << datafile.c_str() << std::endl;
    return 1;
  }
  DAQ_header.ReadHeaderContent();

  
  ProcessEvents(DAQ_header, argv[1]);
  
  return 1;
}
