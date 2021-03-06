/*
  Event viewer code for data from UCLA EXACT TPCs. Can overlay analysis
  
  Compile with Makefile
  
  v0.1 AFan 2013-04-21
    - base code adapted from tpcanalysis.cc
  v0.2 AFan 2013-08-17
    - TApplication works as expected now:
    - GUI is responsive, cmd line entry works, event stepping works
  v0.3 AFan 2014-01-21
    - Better event viewer. Uses multithreading.
  v0.4 AFan 2015-01-17
    - Much better event viewer! Uses TEve and GUIs.

 */


#include <iostream>
#include <string>
#include <fstream>

#include <libconfig.h++>

#include "TApplication.h"
#include "TRint.h"
#include "TCanvas.h"
#include "TH1F.h"
#include "TFile.h"
#include "TTree.h"
#include "TBranch.h"
#include "TSystem.h"
#include "TTimer.h"
#include "TRootCanvas.h"

#include "TEveManager.h"
#include "TEveBrowser.h"
#include <TQObject.h>
#include <RQ_OBJECT.h>
#include "TSystem.h"
#include "TGButton.h"
#include "TGNumberEntry.h"
#include "TGLabel.h"
#include "TGLayout.h"

#include "LVDAQHeader.hh"
#include "EventData.hh"
#include "Converter.hh"
#include "BaselineFinder.hh"
#include "ZeroSuppressor.hh"
#include "Integrator.hh"
#include "SumChannel.hh"
#include "PulseFinder.hh"
#include "ProcessedPlotter.hh"
#include "EventProcessor.hh"
#include "EventNavigator.hh"

using namespace std;

// These global variables are instantiated in EventProcessor.cc
extern EventProcessor* gEventProcessor;
extern TGNumberEntryField* gEventNumberEntry;

void make_gui()
{
  TEveBrowser* browser = gEve->GetBrowser();
  browser->StartEmbedding(TRootBrowser::kLeft);

  
  EventNavigator    *eNav = new EventNavigator;

  TGMainFrame* frmMain = new TGMainFrame(gClient->GetRoot(), 1000, 600);
  frmMain->SetWindowName("XX GUI");
  frmMain->SetCleanup(kDeepCleanup);
  
  TString icondir( Form("%s/icons/", gSystem->Getenv("ROOTSYS")) );
  TGLayoutHints* lh = new TGLayoutHints(kLHintsNormal, 0,0,4,0);
  TGHorizontalFrame* hf = new TGHorizontalFrame(frmMain);
  {
    TGPictureButton* b = 0;
    TGLabel* blabel = 0;

    blabel = new TGLabel(hf, " Next ");
    blabel->SetTextJustify(kTextCenterY);
    b = new TGPictureButton(hf, gClient->GetPicture(icondir+"tb_forw.xpm"));
    hf->AddFrame(blabel,lh);
    hf->AddFrame(b);
    b->Connect("Clicked()", "EventNavigator", eNav, "Fwd()");


  }
  frmMain->AddFrame(hf);

  hf = new TGHorizontalFrame(frmMain);
  {
    TGPictureButton* b = 0;
    TGLabel* blabel = 0;

    blabel = new TGLabel(hf, " Prev ");
    blabel->SetTextJustify(kTextCenterY);
    b = new TGPictureButton(hf, gClient->GetPicture(icondir+"tb_back.xpm"));
    hf->AddFrame(blabel, lh);
    hf->AddFrame(b);
    b->Connect("Clicked()", "EventNavigator", eNav, "Bck()");
  }
  frmMain->AddFrame(hf);

  hf = new TGHorizontalFrame(frmMain);
  {
    // ability to jump to event
    TGLabel* eventNbrLabel = new TGLabel(hf, " Go to event # " );
    gEventNumberEntry = new TGNumberEntryField(hf, 0, 0,
                                               TGNumberFormat::kNESInteger,
                                               TGNumberFormat::kNEAPositive,
                                               TGNumberFormat::kNELLimitMinMax,
                                               0, 10000000);
    hf->AddFrame(eventNbrLabel,lh);
    hf->AddFrame(gEventNumberEntry);
    gEventNumberEntry->Connect("ReturnPressed()", "EventNavigator", eNav, "GotoEvent()");
  }
  frmMain->AddFrame(hf);

  
  frmMain->MapSubwindows();
  frmMain->Resize();
  frmMain->MapWindow();

  browser->StopEmbedding();
  browser->SetTabTitle("Event Control", 0);


  
}

//----------------------------------------------------------------
//----------------------------------------------------------------
//----------------------------------------------------------------


int main(int argc, char* argv[]) {

  if (argc<=1) {
    std::cout << "Usage: -c <cfg file> -i <input file>" << std::endl;
    return 1;
  }

  std::string cfgfile;
  std::string datafile;
  
  int opt;
  while ((opt=getopt(argc, argv, "hc:i:")) != -1) {
    switch (opt) {
    case 'h':
      std::cout << "Usage: -c <cfg file> -i <input file>" << std::endl;
      exit(EXIT_SUCCESS);
    case 'c':
      cfgfile = optarg;
      break;
    case 'i':
      datafile = optarg;
      break;
    default:
      std::cout << "Usage: -c <cfg file> -i <input file>" << std::endl;
      exit(EXIT_FAILURE);
    }
  }


  
  // initialize ROOT application
  TApplication *theApp = new TApplication("app", 0, 0);
  theApp->SetReturnFromRun(true);
  //theApp->ProcessFile(".rootstart.C");
  
  TEveManager::Create();

  make_gui();
  
  //string cfgFile = argv[1];

  Config cfg;
  cfg.readFile(cfgfile.c_str());

  
  //string datafile = argv[2];
  
  gEventProcessor = new EventProcessor(cfg);

  gEventProcessor->SetDataFile(datafile);
  
  gEventProcessor->Initialize();


  cout << "Processing event 0"<<endl;
  gEventProcessor->ProcessEvent(0);
  
  theApp->Run();
  
  return 0;
}
