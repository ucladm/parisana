/*
  Event viewer code for data from UCLA EXACT TPCs. Can overlay analysis

  Two parts to code:
  1. Event by event processing, including baseline finding, pulse finding, etc
  2. Apply cuts and generate histograms and average waveforms
  
  Compile with Makefile
  
  v0.1 AFan 2013-04-21
    - base code adapted from tpcanalysis.cc
  v0.2 AFan 2013-08-17
    - TApplication works as expected now:
    - GUI is responsive, cmd line entry works, event stepping works

 */


#include <iostream>
#include <string>
#include <fstream>


#include "TApplication.h"
#include "TCanvas.h"
#include "TH1F.h"
#include "TFile.h"
#include "TTree.h"
#include "TBranch.h"
#include "TSystem.h"
#include "TTimer.h"
#include "TRootCanvas.h"

#include "DAQheader.hh"
#include "CfgReader.hh"
#include "EventData.hh"
#include "testAnalysis.hh"
#include "ChannelData.hh"
#include "Converter.hh"
#include "BaselineFinder.hh"
#include "ProcessedPlotter.hh"
//#include "RootGraphics.hh"
#include "RootGraphix.hh"

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


  // Repopulate these objects for each new event
  EventData* event = new EventData();
  TCanvas* c = new TCanvas("c", "c");

  
  // --------- INSTANTIATE AND INITIALIZE ALL MODULES ------------  
  Converter converter;
  //BaselineFinder baselineFinder("BaselineFinder");
  //darkart::RootGraphics* gr = new darkart::RootGraphics();
  RootGraphix* graphix = new RootGraphix;
  ProcessedPlotter plotter;

  converter.Initialize(cfg); 
  //baselineFinder.Initialize(cfg);

  graphix->Initialize();
  plotter.Initialize(cfg, c, graphix);

  /*
  c = graphix->GetCanvas();
  RootGraphix::Lock glock = graphix->AcquireLock();
  c->Divide(2,2);
  c->cd(1);
  
  std::string test;
  std::cin >> test;
  */
  
  

  int evt = 1;
  std::string line;
  while (line!="q") {
    
    event->Clear();
    event->run_id = 0;
    event->event_id = evt;

    // Run all the modules
    converter.Process(event, DAQ_header);
    //baselineFinder.Process(event);
    plotter.Process(event);
    gPad->Modified();
    gPad->Update();


    // Decide what to do next
    cout << "Enter option: " << endl
         << "  <enter> for next event" << endl
         << "  b for prev event" << endl
         << "  # for event_id #" << endl
         << "  q to quit" << endl;
    
    getline(std::cin, line);

    // read and interpret command line input
    if (line=="")
      evt++;
    else if (line=="b")
      evt--;
    else if (line=="q"||line=="Q")
      break;
    else
      evt = atoi(line.c_str());

  }


  
  
  /*
  // ------------ PLOT AN EVENT WITH SOME ANALYSIS ---------------
  int evt = 1;
  // Use of TTimer allows TApplication to respond to mouse clicks while
  // code continues running.
  TTimer* timer = new TTimer("gSystem->ProcessEvents();", 50, kFALSE);  
  std::string line;
  while (line!="q") {
    
    timer->TurnOn();
    timer->Reset();

    event->Clear();
    event->run_id = 0;
    event->event_id = evt;

    // Run all the modules
    converter.Process(event, DAQ_header);
    //baselineFinder.Process(event);
    plotter.Process(event);
    gPad->Modified();
    gPad->Update();


    // Decide what to do next
    cout << "Enter option: " << endl
         << "  <enter> for next event" << endl
         << "  b for prev event" << endl
         << "  # for event_id #" << endl
         << "  q to quit" << endl;
    
    getline(std::cin, line);

    // read and interpret command line input
    if (line=="")
      evt++;
    else if (line=="b")
      evt--;
    else if (line=="q"||line=="Q")
      break;
    else
      evt = atoi(line.c_str());

    timer->TurnOff();      
  }
  */
  graphix->Finalize();
  if (c) delete c;
  return 1;
}


//----------------------------------------------------------------
//----------------------------------------------------------------
//----------------------------------------------------------------


int main(int args, char* argv[]) {

  if (args!=3) {
    std::cout << "Use correct command: ./eventviewer <cfg file> <rawdatafile>"
              << std::endl;
    return 1;
  }
  
  // initialize ROOT application
  TApplication *theApp = new TApplication("app", 0, 0);
  theApp->SetReturnFromRun(true);
  //theApp->ProcessFile(".rootstart.C");

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

  return ProcessEvents(DAQ_header, argv[1]);
}
