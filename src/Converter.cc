#include "Converter.hh"

#include <iostream>
#include <map>
#include <algorithm> //sort
#include <cmath> //fabs

Converter::Converter(CfgReader const& cfg):
  module_name("Converter"),
  _enabled(cfg.getParam<bool>(module_name, "enabled", true)),
  _trigger_index_offset(cfg.getParam<int>(module_name, "trigger_index_offset", 0))
{ }


int Converter::Process(EventData* event, DAQheader & DAQ_header)
{
  if (!_enabled)
    return 0;
  
  // Event ID should already be set
  if (event->event_id == -1) {
    std::cerr << "Converter: event_id not set yet!" << std::endl;
    return 0;
  }
  

  // Fill event-level info
  event->nchans = DAQ_header.getNchans();
  event->nsamps = DAQ_header.getTotSampleNbr();
  event->us_per_samp = DAQ_header.getTimeInterval_us();
  event->trigger_index = DAQ_header.getTriggerIndex();
  event->trigger_index_offset = _trigger_index_offset;

  // hard code these for now. should (will?) be added to DAQ header later.
  event->adc_bits = 8;
  event->adc_range_top = 0.2;
  event->adc_range_bot = 2;

    
    //----------------------------
    
    const double BOT_PMT_ConvertFactor = 10/1.602177/25; //--- convert mV*ns to 1E6 e-, with 25 ohm impadence ------
    const double TOP_PMT_ConvertFactor = 10/1.602177/50; //--- convert mV*ns to 1E6 e-, with 25 ohm impadence ------

    const double btm_PMT_Gain = 4.064; //--- unit: 1E6 ---
    
    /*
    --- channel order ----
     Channel#0 3" PMT   Channel#1 LV1556
     Channel#2 LV1549   Channel#3 LV1548
     Channel#4 LV1550   Channel#5 LV
     
    */
    
    const double top_PMT_Gain[7] = {}; //--- unit: 1E6, follows the cable order ---

    //----------------------------
    
  // Fill channel-level info
  for (int i=0; i<DAQ_header.getNchans(); ++i) {
      
    event->channel_nums.push_back(DAQ_header.WorkingChannelNbr.at(i));
    event->channel_ids.push_back(i);
    event->raw_waveforms.push_back(DAQ_header.ReadSingleChannel(event->event_id, DAQ_header.WorkingChannelNbr.at(i)));
    
    if(i==0)//--- assuming top PMT is always the channel#0 ---
    event->spe_means.push_back(btm_PMT_Gain/2/BOT_PMT_ConvertFactor);
    else
    event->spe_means.push_back(top_PMT_Gain[i+1]/2/TOP_PMT_ConvertFactor);
      
  }

  
  // Check that channel numbers match first event
  if (event->event_id == 1) {
    for (int i=0; i<event->nchans; i++) {
      _initial_channel_nums.push_back(event->channel_nums[i]);
    }
  }
  else {
    for (int i=0; i<event->nchans; i++) {
      if (event->channel_nums[i] != _initial_channel_nums[i])
        std::cerr << "Event "<<event->event_id<<" channels in wrong order!"<<std::endl;
    }
  }
  
  
  // Trim the waveforms so that they are aligned.
  // _trigger_index_offset is top channels with respect to bottom channel.
  // If >0, trim start of top channels, and end of bottom channel.
  // If <0, trim start of bottom channel, and end of top channels.
  for (int ch=0; ch<event->nchans; ch++) {
    std::vector<double> & waveform = event->raw_waveforms[ch];
    if (ch==BOT_CHANNEL_ID) {
      if (_trigger_index_offset < 0)
        waveform.erase(waveform.begin(), waveform.begin() - _trigger_index_offset);
      else
        waveform.resize(event->nsamps - _trigger_index_offset);
    }
    else {
      if (_trigger_index_offset < 0)
        waveform.resize(event->nsamps + _trigger_index_offset);
      else
        waveform.erase(waveform.begin(), waveform.begin()+_trigger_index_offset);
    }
  } // end loop over channels
  event->nsamps -= std::fabs(_trigger_index_offset);
  
  return 1;
}
