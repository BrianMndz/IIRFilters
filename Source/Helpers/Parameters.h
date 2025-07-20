#pragma once
#include <JuceHeader.h>

namespace IIRFilters {

    static const juce::String GainDb = "Gain";
    static const juce::String cutoffFrequency = "Cutoff_Frequency";
    static const juce::String qVal = "Q";
    static const juce::String filterType = "Filter_Type";

    inline juce::StringArray FilterTypeChoices = {
        "LPF1P", "LPF1", "HPF1", "LPF2", "HPF2", "BPF2", "BSF2", "ButterLPF2", "ButterHPF2", "ButterBPF2",
        "ButterBSF2", "MMALPF2", "MMALPF2B", "LowShelf", "HiShelf", "NCQParaEQ", "CQParaEQ", "LWRLPF2", "LWRHPF2",
        "APF1", "APF2", "ResonA", "ResonB", "MatchLP2A", "MatchLP2B", "MatchBP2A", "MatchBP2B",
        "ImpInvLP1", "ImpInvLP2"
    };

    //==============================================================================
    inline juce::AudioProcessorValueTreeState::ParameterLayout createParameters()
    {
        juce::AudioProcessorValueTreeState::ParameterLayout params;

        params.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID (GainDb, 1), GainDb, -18.0f, 18.0f, 0.0f));

        params.add (std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID(filterType, 1), "Filter Type", FilterTypeChoices, 3));

        // Cutoff Frequency Parameter (20Hz to 20kHz, logarithmic scale)
        params.add (std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(cutoffFrequency, 1), cutoffFrequency,
            juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.25f), 1000.0f));

        // Q Parameter (0.1 to 18, linear scale)
        params.add (std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(qVal, 1), qVal,
            juce::NormalisableRange<float>(0.1f, 18.0f, 0.01f), 0.707f));

        return params;
    }

}