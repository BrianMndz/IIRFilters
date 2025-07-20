#pragma once

#include <JuceHeader.h>
#include "dsp/AudioFilter.h"

//==============================================================================
class AudioPluginAudioProcessor final : public juce::AudioProcessor
{
public:
    //==============================================================================
    AudioPluginAudioProcessor();
    ~AudioPluginAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

private:
    //==============================================================================
    // Parameter Layout Creation
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    
    // JUCE Parameters
    juce::AudioProcessorValueTreeState apvts;
    
    // DSP Objects - separate instances for each channel
    wpdsp::AudioFilter leftFilter;
    wpdsp::AudioFilter rightFilter;
    
    // Parameter smoothing for sample-accurate changes
    juce::SmoothedValue<double> smoothedCutoff;
    juce::SmoothedValue<double> smoothedQ;
    juce::SmoothedValue<double> smoothedGain;
    
    // Track last algorithm to avoid unnecessary updates
    wpdsp::FilterAlgorithm lastAlgorithm = wpdsp::FilterAlgorithm::kLPF2;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessor)
};
