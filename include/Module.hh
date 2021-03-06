/*
  2015-11-25 AFan

  Base class for all modules.

 */


#ifndef MODULE_HH
#define MODULE_HH

#include <iostream>
#include <libconfig.h++>
#include "TTree.h"
#include "TString.h"

#define NCHANS 8

using namespace libconfig;

class Module
{
public:
  Module(const Setting & cfg);
  ~Module();
  void Initialize();
  void Process();
  void Finalize(TTree* master);

  std::string module_name;
  bool enabled;
  std::vector<int> skip_channels;
  bool skip_channel(int ch);

  TTree* GetTree();

protected:
  TTree* tree;
  bool suppress_tree;
};


#endif
