#pragma once
#include "PluginProcessor.h"
#include <vector>
class FETEditor final:public juce::AudioProcessorEditor,private juce::Timer {
public:
 explicit FETEditor(FETProcessor&);
 void paint(juce::Graphics&) override;
 void resized() override;
private:
 struct Dial {juce::Slider slider;juce::Label label;std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;};
 struct Choice {juce::ComboBox combo;juce::Label label;std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attachment;};
 FETProcessor& processor;
 std::array<Dial,10> dials;
 std::array<Choice,6> choices;
 juce::ToggleButton clean,linkIron;
 std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> cleanAttachment,linkAttachment;
 void timerCallback() override {repaint();}
 void updateIron();
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FETEditor)
};
