#include "EventData.hh"
#include "TGraph.h"
#include "TMultiGraph.h"
#include "TAxis.h"
#include "TLine.h"
#include "TBox.h"
#include "TPad.h"
#include "TList.h"
#include "TGaxis.h"
#include "TMarker.h"
#include "TStyle.h"
#include <complex> //abs
#include <algorithm> //min_element
#include <iostream>  // std::cerr
#include <stdexcept> // std::out_of_range
#include <vector>

EventData::EventData() :
  // event level metadata
  run_id(-1),
  event_id(-1),
  nchans(-1),
  nsamps(-1),
  us_per_samp(-1),
  trigger_index(-1),
  trigger_index_offset(-1),
  adc_bits(-1),

  channels(),
  sumchannel(NULL),
  npulses(-1),
  pulses(),
  roi(0)

{}

void EventData::Clear()
{
  run_id = -1;
  event_id = -1;
  nchans = -1;
  nsamps = -1;
  us_per_samp = -1;
  trigger_index = -1;
  trigger_index_offset = -1;
  adc_bits = -1;
  channels.clear();
  //sumchannel = NULL;
  npulses = 0;
  pulses.clear();
  roi = -1;
}


double EventData::SampleToTime(int samp) const
{
  return (samp-trigger_index)*us_per_samp;
}

int EventData::TimeToSample(double time, bool checkrange) const
{
  int samp = (int)(time/us_per_samp)+trigger_index;
  if (samp > nsamps)
    samp = nsamps;
  else if (samp < 0)
    samp = 0;

  return samp;
}


ChannelData* EventData::GetChannel(int const channel_id)
{
  //if (channels[channel_id]->channel_id != channel_id)
  //  std::cout << "Requesting non-existent channel!" << std::endl;
  //return channels[channel_id];
  std::vector<ChannelData>::iterator it = channels.begin();
  while (it != channels.end() && it->channel_id != channel_id) it++;
  return (it == channels.end() ? 0 : &(*it));
}

TMultiGraph* EventData::GetTMultiGraph(int ch)
{
  ChannelData* channel = GetChannel(ch);

  
  char name[30];
  sprintf(name, "r%ie%ich%i", run_id, event_id, channel->channel_id);
  TMultiGraph* mg = new TMultiGraph(name, name);

  
  // set the x axis
  std::vector<double> x(nsamps);
  for(int i=0; i<nsamps; i++)
    x[i] = (i - trigger_index) * us_per_samp;


  try {
    std::vector<double> const& raw = channel->raw_waveform;
    //std::vector<double> const& raw = channel->baseline_subtracted_waveform;
    TGraph* gr_raw = new TGraph(nsamps, &x[0], &raw[0]);
    gr_raw->SetTitle(name);
    gr_raw->SetName(name);
    gr_raw->SetMarkerStyle(7);
    mg->Add(gr_raw);
    
    mg->Draw("alp");
    mg->GetXaxis()->SetTitle("time [#mus]");
    mg->GetYaxis()->SetTitle("amp [counts]");
  }
  catch (const std::out_of_range& oor) {
    //std::cerr << "Out of Range error: " << oor.what() << '\n';
  }
  
  try { 
    std::vector<double> baseline = channel->baseline_subtracted_waveform;
    std::vector<double> const& raw = channel->raw_waveform;
    for (size_t i=0; i<baseline.size(); i++)
      baseline[i] = raw[i] - baseline[i];
    TGraph* gr_baseline = new TGraph(nsamps, &x[0], &baseline[0]);
    gr_baseline->SetMarkerColor(kRed);
    gr_baseline->SetLineColor(kRed);
    mg->Add(gr_baseline);
  }
  catch (const std::out_of_range& oor) {
    //std::cerr << "Out of Range error: " << oor.what() << '\n';
  }

  
  // need to adjust size of integral so it fits
  try {
    std::vector<double> adjusted_integral(nsamps); 
    double integral_offset = channel->baseline_mean; //(draw_baseline_subtracted ? 0 : bs_info.mean);
    double x1,x2,y1,y2;
    gPad->Update();
    gPad->GetRangeAxis(x1,y1,x2,y2);
    double raw_ratio = (y2 - integral_offset) / (integral_offset - y1);
    double integral_max = *std::max_element(channel->integral_waveform.begin(), channel->integral_waveform.end());
    double integral_min = *std::min_element(channel->integral_waveform.begin(), channel->integral_waveform.end());
    double integral_ratio = std::abs(integral_max) / std::abs(integral_min);
    double integral_scale;
    if (raw_ratio < integral_ratio)
      integral_scale = (y2 - integral_offset) / std::abs(integral_max) * 0.9;
    else
      integral_scale = (integral_offset - y1) / std::abs(integral_min) * 0.9;
  
  
    for(int i = 0; i<nsamps; i++)
      adjusted_integral[i] = integral_scale*channel->integral_waveform[i] + integral_offset;
  
    int integral_color = kBlue;
  
    TGraph* integral_gr = new TGraph(nsamps, &x[0], &adjusted_integral[0]);
    integral_gr->SetLineColor(integral_color);
    integral_gr->SetMarkerColor(integral_color);
    mg->Add(integral_gr);
  }
  catch (const std::out_of_range& oor) {
    //std::cerr << "Out of Range error: " << oor.what() << '\n';
  }


  
  return mg;
}


TMultiGraph* EventData::GetTMultiGraph_sumch()
{

  char name[30];
  sprintf(name, "r%ie%ichSUM", run_id, event_id);
  TMultiGraph* mg = new TMultiGraph(name, name);
  
  if (sumchannel->raw_waveform.empty())
    return mg;
  int nsamps = sumchannel->raw_waveform.size();
  std::vector<double> x(nsamps);


  // set the x axis
  for(int i=0; i<nsamps; i++)
    x[i] = (i - trigger_index) * us_per_samp;

  TGraph* gr_raw = new TGraph(nsamps, &x[0], &sumchannel->raw_waveform[0]);
  gr_raw->SetTitle(name);
  gr_raw->SetName(name);
  mg->Add(gr_raw);

  mg->Draw("alp");
  mg->GetXaxis()->SetTitle("time [#mus]");
  mg->GetYaxis()->SetTitle("amp [arb]");

  
  
  
  std::vector<double> baseline = sumchannel->raw_waveform;
  for (size_t i=0; i<baseline.size(); i++)
    baseline[i] = sumchannel->raw_waveform[i] - baseline[i];
  TGraph* gr_baseline = new TGraph(nsamps, &x[0], &baseline[0]);
  gr_baseline->SetMarkerColor(kRed);
  gr_baseline->SetLineColor(kRed);
  mg->Add(gr_baseline);


  if (sumchannel->integral_waveform.empty())
    return mg;
  
  // need to adjust size of integral so it fits
  std::vector<double> adjusted_integral(nsamps); 
  double integral_offset = 0; 
  double x1,x2,y1,y2;
  gPad->Update();
  gPad->GetRangeAxis(x1,y1,x2,y2);
  double raw_ratio = (y2 - integral_offset) / (integral_offset - y1);
  double integral_max = *std::max_element(sumchannel->integral_waveform.begin(), sumchannel->integral_waveform.end());
  double integral_min = *std::min_element(sumchannel->integral_waveform.begin(), sumchannel->integral_waveform.end());
  double integral_ratio = std::abs(integral_max) / std::abs(integral_min);
  double integral_scale;
  if (raw_ratio < integral_ratio)
    integral_scale = (y2 - integral_offset) / std::abs(integral_max) * 0.9;
  else
    integral_scale = (integral_offset - y1) / std::abs(integral_min) * 0.9;
  
  
  for(int i = 0; i<nsamps; i++)
    adjusted_integral[i] = integral_scale*sumchannel->integral_waveform[i] + integral_offset;
  
  int integral_color = kBlue;
  
  TGraph* integral_gr = new TGraph(nsamps, &x[0], &adjusted_integral[0]);
  integral_gr->SetLineColor(integral_color);
  integral_gr->SetMarkerColor(integral_color);
  mg->Add(integral_gr);
  

  for (int i=0; i<npulses; i++) {
    double base = 0;
    double peak_y = base + pulses[i]->peak_amp;
    TBox* pbox = new TBox( pulses[i]->start_time, base,
                           pulses[i]->end_time, peak_y );
    pbox->SetBit(TObject::kCanDelete,true);
    pbox->SetLineColor(kGreen);
    pbox->SetFillStyle(0);
    pbox->Draw();
    //TLine* pline = new TLine( x[pulse_param.peak_index], base,
    //                          x[pulse_param.peak_index], peak_y);
    //pline->SetBit(TObject::kCanDelete,true);
    //pline->SetLineColor(kMagenta);
    //pline->Draw();
  }//end loop over pulses

  
  return mg;
}

