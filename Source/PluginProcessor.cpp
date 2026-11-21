#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>
namespace {
float db(float d){return juce::Decibels::decibelsToGain(d);}
float pole(float hz,double sr){return std::exp(-2.f*juce::MathConstants<float>::pi*hz/(float)sr);}
float tc(float sec,double sr){return std::exp(-1.f/juce::jmax(1.f,sec*(float)sr));}
float kneeGain(float over,float ratio,float width){
 const float slope=1.f-1.f/ratio;
 if(width<=0.f)return slope*juce::jmax(0.f,over);
 if(over<=-width*.5f)return 0.f;
 if(over>=width*.5f)return slope*over;
 const float v=over+width*.5f;
 return slope*v*v/(2.f*width);
}
}
juce::AudioProcessorValueTreeState::ParameterLayout FETProcessor::layout(){
 using F=juce::AudioParameterFloat;using C=juce::AudioParameterChoice;using B=juce::AudioParameterBool;
 juce::AudioProcessorValueTreeState::ParameterLayout p;
 auto f=[&](const char* id,const char* name,float lo,float hi,float def){p.add(std::make_unique<F>(juce::ParameterID{id,1},name,juce::NormalisableRange<float>(lo,hi,.1f),def));};
 f("input","Input (dB)",-24,24,0);f("output","Output (dB)",-24,24,0);f("drive","FET Drive (%)",0,100,50);f("mix","Mix (%)",0,100,100);
 f("attack","Attack (clockwise faster)",0,100,50);f("release","Release (clockwise faster)",0,100,55);
 p.add(std::make_unique<C>(juce::ParameterID{"revision",1},"Revision",juce::StringArray{"Rev A","Rev F","Rev G"},1));
 p.add(std::make_unique<C>(juce::ParameterID{"ratio",1},"Ratio",juce::StringArray{"4:1","8:1","12:1","20:1","ALL BUTTONS IN"},0));
 p.add(std::make_unique<C>(juce::ParameterID{"timing",1},"Timing",juce::StringArray{"1176 Classic","Extended"},0));
 p.add(std::make_unique<C>(juce::ParameterID{"knee",1},"Knee",juce::StringArray{"1176-style","LA-2A-inspired","SSL G Bus-inspired"},0));
 p.add(std::make_unique<B>(juce::ParameterID{"clean",1},"Clean (bypass harmonic modelling)",false));
 p.add(std::make_unique<B>(juce::ParameterID{"linkIron",1},"Link input/output circuits to revision",true));
 p.add(std::make_unique<C>(juce::ParameterID{"inputIron",1},"Input Stage",juce::StringArray{"Rev A Peerless-style","Rev F UTC-style","Rev G Differential","Bypass"},1));
 p.add(std::make_unique<C>(juce::ParameterID{"outputIron",1},"Output Stage",juce::StringArray{"Rev A UA-5002-style","Rev F B11148-style","Rev G B11148-style","Bypass"},1));
 f("scHPF","Sidechain HPF (Hz)",20,400,20);
 f("scFreq","Sidechain Parametric Frequency (Hz)",50,12000,1000);
 f("scGain","Sidechain Parametric Gain (dB)",-18,18,0);
 f("scQ","Sidechain Parametric Q",.2,8,1);
 return p;
}
FETProcessor::FETProcessor():AudioProcessor(BusesProperties().withInput("Input",juce::AudioChannelSet::stereo(),true).withOutput("Output",juce::AudioChannelSet::stereo(),true)),parameters(*this,nullptr,"STATE",layout()){}
void FETProcessor::prepareToPlay(double sr,int){sampleRate=sr;states={};setLatencySamples(0);}
void FETProcessor::processBlock(juce::AudioBuffer<float>& buffer,juce::MidiBuffer&){
 juce::ScopedNoDenormals noDenormals;
 const int n=buffer.getNumSamples(),channels=juce::jmin(2,buffer.getNumChannels());
 const auto v=[&](const char* id)->float{return parameters.getRawParameterValue(id)->load(std::memory_order_relaxed);};
 const float input=db(v("input")),output=db(v("output")),drive=v("drive")*.01f,mix=v("mix")*.01f;
 const int revision=juce::jlimit(0,2,(int)v("revision"));
 const int ratioChoice=juce::jlimit(0,4,(int)v("ratio"));
 const bool allButtons=ratioChoice==4;
 const float ratios[4]={4,8,12,20};
 // All-buttons mode deliberately has its own nonlinear transfer curve,
 // not a fictitious 36:1 or other fixed ratio.
 const float ratio=ratios[allButtons?3:ratioChoice];
 const bool extended=v("timing")>.5f,clean=v("clean")>.5f,linked=v("linkIron")>.5f;
 const int inModel=linked?revision:juce::jlimit(0,3,(int)v("inputIron"));
 const int outModel=linked?revision:juce::jlimit(0,3,(int)v("outputIron"));
 const float attackPosition=v("attack")*.01f,releasePosition=v("release")*.01f;
 const float slowAttack=extended?.030f:.0008f,fastRelease=extended?.025f:.050f,slowRelease=extended?1.650f:1.100f;
 const float ac=tc(slowAttack*std::pow(.000020f/slowAttack,attackPosition),sampleRate);
 const float rc=tc(fastRelease*std::pow(slowRelease/fastRelease,1.f-releasePosition),sampleRate);
 const float width=(int)v("knee")==0?5.f:((int)v("knee")==1?14.f:9.f);
 const float scHPF=v("scHPF"),scFreq=v("scFreq"),scGain=v("scGain"),scQ=v("scQ");
 const float hpAlpha=1.f/(1.f+2.f*juce::MathConstants<float>::pi*scHPF/(float)sampleRate);
 // Stable state-variable sidechain bell filter, detector-only (audio path unaffected).
 const float g=std::tan(juce::MathConstants<float>::pi*juce::jmin(scFreq,(float)sampleRate*.45f)/(float)sampleRate);
 const float q=juce::jmax(.2f,scQ),r=1.f/q;
 const float bell=db(scGain)-1.f;
 // Drive is a colour-only offset. 50% is unity/neutral; detector and gain reduction are independent.
 const float colourDb=(drive-.5f)*24.f;
 const float colourGain=db(colourDb);
 const float colourTrim=db(-colourDb);
 const float inputLowPole=pole(110,sampleRate),inputFluxPole=pole(35,sampleRate),inputHighPole=pole(12000,sampleRate);
 const float outputLowPole=pole(155,sampleRate),outputFluxPole=pole(43,sampleRate),outputHighPole=pole(12500,sampleRate);
 const float ampPole=pole(8500,sampleRate),biasPole=pole(24,sampleRate);
 float totalGR=0;
 for(int c=0;c<channels;++c){
  auto& s=states[(size_t)c];auto* samples=buffer.getWritePointer(c);
  for(int i=0;i<n;++i){
   const float dry=samples[i]*input;
   const float hot=dry;
   float coloured=hot;
   const float driven=hot*colourGain;
   if(!clean&&inModel!=3){
    if(inModel!=2){
     s.inputLow=inputLowPole*s.inputLow+(1-inputLowPole)*driven;
     s.inputFlux=inputFluxPole*s.inputFlux+(1-inputFluxPole)*s.inputLow;
     const float strength=inModel==0?.17f:.10f;
     const float d=(inModel==0?2.1f:1.6f)*(1.f+.25f*juce::jmin(2.f,std::abs(s.inputFlux)));
     coloured+=colourTrim*strength*(std::tanh(d*s.inputLow)/d-s.inputLow);
     s.inputHigh=inputHighPole*s.inputHigh+(1-inputHighPole)*coloured;
     coloured-=.015f*juce::jmin(1.f,std::abs(s.inputFlux))*(coloured-s.inputHigh);
    }else {
     // Rev G differential input is the cleanest option: no soft clipper.
     // Leave the signal linear here; output amplifier adds its own colour.
     coloured=hot;
    }
   }
   float detected=hpAlpha*(s.scHPy+coloured-s.scHPx);
   s.scHPx=coloured;s.scHPy=detected;
   // TPT state-variable band-pass for the sidechain parametric bell.
   const float a1=1.f/(1.f+g*(g+r));
   const float hp=(detected-(r+g)*s.scBand-s.scLow)*a1;
   const float bp=g*hp+s.scBand;
   const float lp=g*bp+s.scLow;
   s.scBand=g*hp+bp;s.scLow=g*bp+lp;
   detected+=bell*bp;
   const float mag=std::abs(detected);
   const float k=mag>s.env?ac:rc;
   s.env=k*s.env+(1.f-k)*mag;
   const float level=juce::Decibels::gainToDecibels(juce::jmax(s.env,1e-8f),-160.f);
   float gr=kneeGain(level+18.f,ratio,width);
   if(allButtons){
    // Inspired by the 1176 all-ratios-in interaction: irregular knee,
    // stronger gain reduction and additional envelope-dependent movement.
    const float over=level+18.f;
    const float push=juce::jmax(0.f,over);
    gr=kneeGain(over,12.f,11.f)+.20f*push*push/(12.f+push);
    gr=juce::jlimit(0.f,48.f,gr);
   }
   totalGR+=gr;
   coloured*=db(-gr);
   if(!clean){
    // Rev F and G share the later push-pull Class AB output topology.
    // Rev G differs primarily in its transformerless differential input.
    // Hardware-style ordering: input level sets detector and input-stage
    // saturation; FET attenuation then determines the actual signal feeding
    // the output amplifier and transformer. No makeup/recovery is inserted
    // before these nonlinear stages. FET Drive remains colour-only.
    const float ampDrive=(revision==0?1.9f:1.4f)*(allButtons?1.35f:1.f)*colourGain;
    const float ampBias=revision==0?.095f:(revision==1?.035f:.020f);
    s.ampBias=biasPole*s.ampBias+(1-biasPole)*std::abs(coloured);
    const float bias=ampBias*juce::jmax(0.f,drive-.5f)*2.f*s.ampBias;
    const float z=coloured*ampDrive+bias;
    const float ampSaturated=(std::tanh(z)-std::tanh(bias))/juce::jmax(.01f,ampDrive);
    // The F/G push-pull amplifier is cleaner than the Rev A Class A circuit.
    // Preserve the linear path and mix only a small amount of nonlinear colour.
    const float ampColour=revision==0?1.f:.20f;
    coloured=coloured*(1.f-ampColour)+ampSaturated*ampColour;
    s.ampMemory=ampPole*s.ampMemory+(1-ampPole)*coloured;
    coloured-=(revision==0?.075f:.025f)*juce::jmax(0.f,drive-.5f)*2.f*(coloured-s.ampMemory);
    if(outModel!=3){
     s.outputLow=outputLowPole*s.outputLow+(1-outputLowPole)*coloured;
     s.outputFlux=outputFluxPole*s.outputFlux+(1-outputFluxPole)*s.outputLow;
     // Both Rev F and Rev G use the B11148-style output transformer.
     const float strength=outModel==0?.16f:.065f;
     const float d=outModel==0?1.75f:1.35f;
     coloured+=strength*(std::tanh(d*s.outputLow)/d-s.outputLow);
     s.outputHigh=outputHighPole*s.outputHigh+(1-outputHighPole)*coloured;
     coloured-=.02f*juce::jmin(1.f,std::abs(s.outputFlux))*(coloured-s.outputHigh);
    }
   }
   // Post-output-stage makeup only: cannot affect amplifier/transformer saturation.
   const float targetMakeup=juce::jlimit(0.f,9.f,gr*.65f);
   const float mc=tc(targetMakeup>s.makeupDb?.18f:.50f,sampleRate);
   s.makeupDb=mc*s.makeupDb+(1.f-mc)*targetMakeup;
   const float postStageMakeup=db(s.makeupDb);
   const float wet=coloured*postStageMakeup;
   // Output trim is the final gain stage, after output transformer and makeup.
   samples[i]=(wet*mix+dry*(1.f-mix))*output;
  }
 }
 for(int c=channels;c<buffer.getNumChannels();++c)buffer.clear(c,0,n);
 gainReduction.store(totalGR/juce::jmax(1,n*channels),std::memory_order_relaxed);
}
juce::AudioProcessorEditor* FETProcessor::createEditor(){return new FETEditor(*this);}
void FETProcessor::getStateInformation(juce::MemoryBlock& b){auto xml=parameters.copyState().createXml();copyXmlToBinary(*xml,b);}
void FETProcessor::setStateInformation(const void* data,int size){if(auto xml=getXmlFromBinary(data,size))if(xml->hasTagName(parameters.state.getType()))parameters.replaceState(juce::ValueTree::fromXml(*xml));}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new FETProcessor();}
