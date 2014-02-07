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

  // Fill channel-level info
  // DAQ channel numbers are 1-indexed. Want global channel IDs to be 0-indexed. Presumably, DAQ
  // channel numbers are in order. Check anyway.
  for (int i=0; i<DAQ_header.getNchans(); ++i) {
    event->channel_nums.push_back(DAQ_header.WorkingChannelNbr.at(i));
    event->channel_ids.push_back(i);
    if (DAQ_header.WorkingChannelNbr.at(i) != i+1) {
      std::cerr << "Event "<<event->event_id<<": Unexpected channel order!"<<std::endl;
      abort();
    }
    event->spe_means.push_back(1);
    double gain, offset;
    event->raw_waveforms.push_back(DAQ_header.ReadSingleChannel(event->event_id,
                                                                DAQ_header.WorkingChannelNbr.at(i),
                                                                gain,offset));
    event->adc_gains.push_back(gain);
    event->adc_offsets.push_back(offset);
    event->adc_ranges.push_back(DAQ_header.WorkingChannelFullScale.at(i));
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
