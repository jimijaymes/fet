#pragma once
#include <JuceHeader.h>
#include <array>
#include <atomic>
class FETProcessor final : public juce::AudioProcessor {
public:
 FETProcessor();
 void prepareToPlay(double,int) override;
 void releaseResources() override {}
 void processBlock(juce::AudioBuffer<float>&,juce::MidiBuffer&) override;
 juce::AudioProcessorEditor* createEditor() override;
 bool hasEditor() const override {return true;}
 const juce::String getName() const override {return "FET";}
 bool acceptsMidi() const override {return false;}
 bool producesMidi() const override {return false;}
 bool isMidiEffect() const override {return false;}
 double getTailLengthSeconds() const override {return 0;}
 int getNumPrograms() override {return 1;}
 int getCurrentProgram() override {return 0;}
 void setCurrentProgram(int) override {}
 const juce::String getProgramName(int) override {return {};}
 void changeProgramName(int,const juce::String&) override {}
 void getStateInformation(juce::MemoryBlock&) override;
 void setStateInformation(const void*,int) override;
 juce::AudioProcessorValueTreeState parameters;
 std::atomic<float> gainReduction{0};
private:
 static juce::AudioProcessorValueTreeState::ParameterLayout layout();
 struct State {
  float env=0,scHPx=0,scHPy=0,scLow=0,scBand=0,makeupDb=0;
  float inputLow=0,inputFlux=0,inputHigh=0,inputCoreLow=0,inputCoreHigh=0;
  float ampBias=0,ampMemory=0;
  float outputLow=0,outputFlux=0,outputHigh=0,outputCoreLow=0,outputCoreHigh=0;
 };
 std::array<State,2> states{};
 double sampleRate=44100;
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FETProcessor)
};
