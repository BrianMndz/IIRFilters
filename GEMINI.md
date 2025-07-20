# Plan: Integrating DSP Classes with JUCE Processor

This document provides a step-by-step guide for connecting the framework-agnostic `AudioFilter` and `Biquad` classes to the JUCE `PluginProcessor`. The goal is to use modern C++ best practices, manage parameters with `juce::AudioProcessorValueTreeState`, and ensure sample-accurate parameter updates using smoothing.

## Part 1: Improvements for Ported DSP Classes

Before integration, we can make a few small improvements to the `Biquad` and `AudioFilter` classes to make them more robust and modern.

### 1. `Biquad.h` Enhancements

The `Biquad.h` header is already well-designed. Here are two minor suggestions:

*   **Use `std::array` for `setCoefficients`:** Change the `setCoefficients` method to accept a `std::array` by `const` reference. This is more type-safe than a raw C-style pointer and prevents potential bugs from passing an array of the wrong size.
*   **Add `[[nodiscard]]`:** Add the `[[nodiscard]]` attribute to `processAudioSample`. This is a modern C++ feature (since C++17) that instructs the compiler to issue a warning if the function's return value is ignored. For a function like `processAudioSample`, which exists solely to compute and return a new value, ignoring the result is almost always a bug. This attribute helps prevent such errors by making the programmer's intent clear.

**Example `Biquad.h`:**
```cpp
#pragma once

#include <array>
#include <cmath>

namespace wpdsp {

    enum FilterCoeff { a0, a1, a2, b1, b2, c0, d0, numCoeffs };
    enum StateReg { x_z1, x_z2, y_z1, y_z2, numStates };
    enum class BiquadAlgorithm { kDirect, kCanonical, kTransposeDirect, kTransposeCanonical };

    class Biquad {
    public:
        Biquad() = default;
        ~Biquad() = default;

        void reset();

        [[nodiscard]] double processAudioSample (double sample);

        // Changed to accept std::array by const reference
        void setCoefficients (const std::array<double, numCoeffs>& newCoefficients);

        void setAlgorithm (BiquadAlgorithm newAlgo);

    private:
        std::array<double, numCoeffs> m_coefficients{};
        std::array<double, numStates> m_state{};
        BiquadAlgorithm m_biquadAlgorithm = BiquadAlgorithm::kDirect;
    };
}
```

### 2. `AudioFilter.h` Refinements

The `planport.md` suggests a good, framework-agnostic structure. However, the original `AudioFilter` had special behavior for shelving/EQ filters that used the `c0` (wet) and `d0` (dry) coefficients to mix the filtered and dry signals. The `Biquad` itself only handles the core recursive filtering. The `AudioFilter` must manage this mixing.

*   **Use Descriptive Names:** Instead of the original `c0` and `d0`, use more descriptive names like `m_wet` and `m_dry`. This makes the code self-documenting and easier to understand.
*   **Store Wet/Dry Coefficients:** The `AudioFilter` should store `m_wet` and `m_dry` as member variables.
*   **Update `recalculateCoefficients`:** This method should calculate all coefficients. It will pass the `a` and `b` coefficients to its internal `Biquad` instance and store the wet/dry coefficients locally.
*   **Implement Mixing in `processAudioSample`:** The `processAudioSample` method must implement the formula `output = m_dry * input + m_wet * biquad.processAudioSample(input)`.

**Example `AudioFilter.h`:**
```cpp
#pragma once

#include "Biquad.h"
#include <vector> // Or some other container for algorithm list

namespace wpdsp {
    // This enum should be defined here or in a shared header.
    // It must contain all filter types from the original fxobjects.h
    enum class FilterAlgorithm { kLPF1, kHPF1, kLPF2, kHPF2, /* ... all others */ };

    class AudioFilter
    {
    public:
        AudioFilter() = default;

        void prepare(double sampleRate);
        void reset();

        // Parameter Setters
        void setAlgorithm(FilterAlgorithm newAlgorithm);
        void setCutoff(double newCutoff);
        void setQ(double newQ);
        void setGainDb(double newGainDb);

        [[nodiscard]] double processAudioSample(double sample);

    private:
        void recalculateCoefficients();

        Biquad m_biquad;
        double m_sampleRate = 44100.0;

        // Internal parameter state
        FilterAlgorithm m_algorithm = FilterAlgorithm::kLPF2;
        double m_cutoff = 1000.0;
        double m_q = 0.707;
        double m_gainDb = 0.0;

        // Store wet and dry coefficients for mixing
        double m_wet = 1.0;
        double m_dry = 0.0;
    };
}
```

## Part 2: Setting Up JUCE Parameters

Now, let's connect the DSP code to JUCE. All parameter management will happen in `PluginProcessor`.

### 1. Add Member Variables (`PluginProcessor.h`)

```cpp
#include "Source/dsp/AudioFilter.h" // Use correct path

class IIRFiltersAudioProcessor : public juce::AudioProcessor
{
    // ...
private:
    // Manages all plugin parameters and their state.
    juce::AudioProcessorValueTreeState m_apvts;

    // Our framework-agnostic filter instance.
    wpdsp::AudioFilter m_filter;

    // Smoothed values for sample-accurate parameter changes.
    juce::SmoothedValue<double> m_smoothedCutoff;
    juce::SmoothedValue<double> m_smoothedQ;
    juce::SmoothedValue<double> m_smoothedGain;

    // Keep track of the last known algorithm to avoid unnecessary recalculations.
    wpdsp::FilterAlgorithm m_lastAlgorithm = wpdsp::FilterAlgorithm::kLPF2;

    // Helper function to create the parameter layout.
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
};
```

### 2. Create Parameter Layout (`PluginProcessor.cpp`)

This function defines all the parameters your plugin will expose to the user and the host.

```cpp
juce::AudioProcessorValueTreeState::ParameterLayout IIRFiltersAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // List of filter types for the dropdown menu
    juce::StringArray filterTypeChoices = { "LPF2", "HPF2", "BPF2", "BSF2", "LowShelf", "HiShelf", "Peak" /* ...add all others */ };

    params.push_back(std::make_unique<juce::AudioParameterChoice>("TYPE", "Filter Type", filterTypeChoices, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("CUTOFF", "Cutoff", juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.25f), 1000.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("Q", "Q", juce::NormalisableRange<float>(0.1f, 18.0f, 0.01f), 0.707f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("GAIN", "Gain", juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), 0.0f));

    return { params.begin(), params.end() };
}
```

### 3. Initialize in the Constructor (`PluginProcessor.cpp`)

In the constructor, call the layout function and initialize the `APVTS`.

```cpp
IIRFiltersAudioProcessor::IIRFiltersAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
#endif
    m_apvts(*this, nullptr, "PARAMETERS", createParameterLayout())
{
    // Add listeners or other setup here if needed
}
```

## Part 3: Processing Audio and Updating Parameters

This is where the "glue" code lives.

### 1. `prepareToPlay()` (`PluginProcessor.cpp`)

Prepare the filter and smoothed values for playback.

```cpp
void IIRFiltersAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    m_filter.prepare(sampleRate);
    m_filter.reset();

    m_smoothedCutoff.reset(sampleRate, 0.05); // 50ms ramp
    m_smoothedQ.reset(sampleRate, 0.05);
    m_smoothedGain.reset(sampleRate, 0.05);
}
```

### 2. `processBlock()` (`PluginProcessor.cpp`)

This is the real-time audio callback.

```cpp
void IIRFiltersAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // 1. Get new parameter values from APVTS
    auto newCutoff = m_apvts.getRawParameterValue("CUTOFF")->load();
    auto newQ = m_apvts.getRawParameterValue("Q")->load();
    auto newGain = m_apvts.getRawParameterValue("GAIN")->load();
    auto newAlgoIndex = static_cast<wpdsp::FilterAlgorithm>(m_apvts.getRawParameterValue("TYPE")->load());

    // 2. Push new values to smoothed helpers
    m_smoothedCutoff.setTargetValue(newCutoff);
    m_smoothedQ.setTargetValue(newQ);
    m_smoothedGain.setTargetValue(newGain);

    // 3. Process audio sample-by-sample
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer (channel);

        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            // 4. Update the filter on each sample with smoothed values
            //    The non-smoothed algorithm is checked first.
            if (newAlgoIndex != m_lastAlgorithm)
            {
                m_filter.setAlgorithm(newAlgoIndex);
                m_lastAlgorithm = newAlgoIndex;
            }
            m_filter.setCutoff(m_smoothedCutoff.getNextValue());
            m_filter.setQ(m_smoothedQ.getNextValue());
            m_filter.setGainDb(m_smoothedGain.getNextValue());

            // 5. Process the sample
            channelData[sample] = m_filter.processAudioSample(channelData[sample]);
        }
    }
}
```

By following this plan, you will have a clean separation between your DSP logic and the JUCE framework, making your code more modular, reusable, and easier to maintain. The use of `AudioProcessorValueTreeState` and `SmoothedValue` ensures your plugin will be robust and behave correctly in a professional audio environment.
