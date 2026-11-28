#include "PluginEditor.h"
namespace {const juce::Colour bg(0xff182027),cream(0xfff2e8d7),gold(0xffd2ab71),cyan(0xff6fc4cf);}
FETEditor::FETEditor(FETProcessor& p):AudioProcessorEditor(p),processor(p){
 const char* ids[]={"input","output","drive","mix","attack","release","scHPF","scFreq","scGain","scQ"};
 const char* labels[]={"INPUT dB","OUTPUT dB","FET DRIVE","MIX","ATTACK","RELEASE","SC HPF Hz","SC BELL Hz","SC BELL dB","SC BELL Q"};
 for(size_t i=0;i<dials.size();++i){
  auto& k=dials[i];k.slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  k.slider.setTextBoxStyle(juce::Slider::TextBoxBelow,false,92,20);
  k.slider.setColour(juce::Slider::rotarySliderFillColourId,i>=6?cyan:gold);
  k.slider.setColour(juce::Slider::rotarySliderOutlineColourId,juce::Colour(0xff39454f));
  k.slider.setColour(juce::Slider::thumbColourId,cream);
  k.slider.setColour(juce::Slider::textBoxTextColourId,cream);
  k.slider.setColour(juce::Slider::textBoxBackgroundColourId,juce::Colours::transparentBlack);
  k.slider.setColour(juce::Slider::textBoxOutlineColourId,juce::Colours::transparentBlack);
  k.label.setText(labels[i],juce::dontSendNotification);
  k.label.setColour(juce::Label::textColourId,cream);
  k.label.setJustificationType(juce::Justification::centred);k.label.setMinimumHorizontalScale(0.68f);
  addAndMakeVisible(k.slider);addAndMakeVisible(k.label);
  k.attachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.parameters,ids[i],k.slider);
 }
 const char* idsC[]={"revision","ratio","timing","knee","inputIron","outputIron"};
 const char* labelsC[]={"REVISION","RATIO","TIMING","KNEE","INPUT CIRCUIT","OUTPUT CIRCUIT"};
 const char* options[][4]={
  {"REV A","REV F","REV G",nullptr},
  {"4:1","8:1","12:1","20:1"},
  {"1176 CLASSIC","EXTENDED",nullptr,nullptr},
  {"1176","LA-2A SOFT","SSL G BUS",nullptr},
  {"A PEERLESS","F UTC","G DIFFERENTIAL","BYPASS"},
  {"A UA-5002","F B11148","G B11148","BYPASS"}
 };
 for(size_t i=0;i<choices.size();++i){
  auto& k=choices[i];
  for(int j=0;j<4;++j)if(options[i][j])k.combo.addItem(options[i][j],j+1);
  if(i==1)k.combo.addItem("ALL BUTTONS IN",5);
  k.combo.setColour(juce::ComboBox::backgroundColourId,juce::Colour(0xff303c45));
  k.combo.setColour(juce::ComboBox::textColourId,cream);
  k.combo.setColour(juce::ComboBox::outlineColourId,i>=4?cyan:gold);
  k.label.setText(labelsC[i],juce::dontSendNotification);
  k.label.setColour(juce::Label::textColourId,cream);
  k.label.setJustificationType(juce::Justification::centred);k.label.setMinimumHorizontalScale(0.68f);
  addAndMakeVisible(k.combo);addAndMakeVisible(k.label);
  k.attachment=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.parameters,idsC[i],k.combo);
 }
 clean.setButtonText("CLEAN / NO HARMONIC COLOUR");
 linkIron.setButtonText("LINK CIRCUITS TO REVISION");
 for(auto* b:{&clean,&linkIron}){b->setColour(juce::ToggleButton::textColourId,cream);b->setColour(juce::ToggleButton::tickColourId,cyan);addAndMakeVisible(*b);}
 cleanAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.parameters,"clean",clean);
 linkAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.parameters,"linkIron",linkIron);
 linkIron.onClick=[this]{updateIron();};
 updateIron();setSize(1120,735);startTimerHz(20);
}
void FETEditor::updateIron(){
 const bool enabled=!linkIron.getToggleState();
 for(size_t i=4;i<6;++i){choices[i].combo.setEnabled(enabled);choices[i].label.setEnabled(enabled);}
}
void FETEditor::paint(juce::Graphics& g){
 g.fillAll(bg);g.setColour(juce::Colour(0xff27323b));g.fillRoundedRectangle(12,12,1096,711,12);
 g.setColour(gold);g.setFont(juce::Font(juce::FontOptions(31,juce::Font::bold)));
 g.drawText("FET  /  REVISION LAB",35,22,700,46,juce::Justification::centredLeft);
 g.setColour(cream);g.setFont(juce::Font(juce::FontOptions(13)));
 g.drawText("REV A / REV F / REV G   •   INDEPENDENT INPUT & OUTPUT CIRCUITS   •   DETECTOR EQ",38,74,1020,24,juce::Justification::centredLeft);
 g.setColour(gold);g.drawRoundedRectangle(28,111,1064,230,9,1.4f);
 g.setColour(cyan);g.drawRoundedRectangle(28,356,1064,174,9,1.4f);
 g.setFont(juce::Font(juce::FontOptions(14,juce::Font::bold)));g.drawText("SIDECHAIN / DETECTOR ONLY",44,357,420,24,juce::Justification::centredLeft);
 g.setColour(cream);g.setFont(juce::Font(juce::FontOptions(12)));
 g.drawText("ATTACK / RELEASE: CLOCKWISE = FASTER  •  EXTENDED: 20 us–30 ms / 25–1650 ms",38,676,1010,23,juce::Justification::centredLeft);
 g.drawText("GAIN REDUCTION: "+juce::String(processor.gainReduction.load(),1)+" dB",38,700,400,22,juce::Justification::centredLeft);
}
void FETEditor::resized(){
 for(int i=0;i<10;++i){
  const int x=i<6?42+i*171:155+(i-6)*212;
  const int y=i<6?131:389;
  auto& k=dials[(size_t)i];
  k.slider.setBounds(x,y,120,105);k.label.setBounds(x-8,y+109,136,23);
 }
 for(int i=0;i<6;++i){
  const int x=45+(i%3)*350,y=i<3?255:465;
  auto& k=choices[(size_t)i];
  // Sidechain row stays reserved for the detector knobs.
  const int yy=i<3?254:540;
  k.label.setBounds(x,yy,285,20);k.combo.setBounds(x,yy+22,285,29);
 }
 clean.setBounds(760,310,285,30);
 linkIron.setBounds(760,620,285,30);
}
