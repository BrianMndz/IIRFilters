#pragma once

#include "PluginProcessor.h"

#include <juce_gui_extra/juce_gui_extra.h>
//#include <juce_audio_processors/processors/juce_AudioProcessorEditor.h>

//==============================================================================
// Simple fallback editor - currently unused since we use GenericAudioProcessorEditor
class IIRFiltersAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit IIRFiltersAudioProcessorEditor (IIRFiltersAudioProcessor&);
    ~IIRFiltersAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    IIRFiltersAudioProcessor& processorRef;
    juce::WebBrowserComponent webBrowserComponent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (IIRFiltersAudioProcessorEditor)
};