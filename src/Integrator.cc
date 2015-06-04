#include "Integrator.hh"
#include "ChannelData.hh"

Integrator::Integrator(CfgReader const& cfg):
  module_name("Integrator"),
  _enabled(cfg.getParam<bool>(module_name, "enabled", true))
{ }


int Integrator::Process(EventData* event)
{
  if (!_enabled)
    return 0;

  //event->integrals.resize(event->nchans);
  
  // Loop over channels
  for (int idx = 0; idx<event->nchans; ++idx) {

    ChannelData* channel = event->GetChannel(idx);
    
//    vector<double> const& bs_wfm = event->baseline_subtracted_waveforms[idx];
      vector<double> const& bs_wfm = channel->zero_suppressed_waveform;


    vector<double> & integral = channel->integral_waveform;
    integrate(bs_wfm, integral);
    
  } // end loop over channels

  //Now integrate the sum channel
  vector<double> const& sum_wfm = event->sumchannel.raw_waveform;
  vector<double> & sum_integral = event->sumchannel.integral_waveform;

  integrate(sum_wfm, sum_integral);
         
  return 1;
}

void Integrator::integrate(std::vector<double> const&  wfm, std::vector<double> & result)
{
  result.resize(wfm.size());
  double sum = 0;
  for (size_t i=0; i<wfm.size(); ++i) {
    result[i] = sum;
    sum += wfm[i];
  }

}
