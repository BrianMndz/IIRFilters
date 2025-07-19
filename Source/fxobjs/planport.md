# Plan: Porting ASPiK IIRFilters to JUCE

This document outlines a plan to port the audio processing objects from `fxobjects.h` into a new JUCE-based project. The primary goals are to modernize the C++ code, improve modularity, and leverage the JUCE framework for building a robust audio plugin.

## 1. Project Setup

1.  **Create JUCE Project:** Use the Projucer to create a new "Audio Plug-In" project named `IIRFiltersJuce`.
2.  **Establish Directory Structure:** Create a `Source/dsp` directory within the JUCE project to hold the ported DSP classes. This will keep the core audio logic separate from the JUCE plugin wrapper code.

## 2. Code Analysis and Refactoring Strategy

The existing `fxobjects.h` is a monolithic header containing dozens of classes, structs, and functions. The first and most critical step is to break this file apart.

### Guiding Principles

*   **One Class, One File:** Each class (e.g., `Biquad`, `AudioFilter`) will be moved into its own header (`.h`) and source (`.cpp`) file.
*   **Namespaces:** All ported code will be placed within a top-level namespace (e.g., `IIRFilters`) to prevent naming conflicts. DSP-specific code can be further nested (e.g., `IIRFilters::DSP`).
*   **Modern C++:**
    *   Replace raw C-style arrays (e.g., `double coeffArray[numCoeffs]`) with `std::array` for fixed-size compile-time arrays or `std::vector` for dynamic ones.
    *   Replace raw pointers with smart pointers (`std::unique_ptr`, `std::shared_ptr`) where ownership is involved.
    *   Use `override` and `final` specifiers on virtual functions to improve clarity and prevent errors.
    *   Use `constexpr` for true compile-time constants.
    *   Favor `enum class` over plain `enum` for type safety (this is already partially done).
*   **JUCE Integration:**
    *   Utilize JUCE's DSP module (`juce::dsp`) and math utilities (`juce::MathConstants`, `juce::dsp::FastMathApproximations`) where they offer a suitable replacement for custom functions. This reduces code maintenance and can offer performance benefits.
    *   The `IAudioSignalProcessor` interface will be retired. The new DSP classes will be designed to work directly with `juce::dsp::ProcessContext` or `juce::AudioBuffer`.
    *   Plugin parameters will be managed by `juce::AudioProcessorValueTreeState`, completely replacing the custom parameter structs for GUI-to-processor communication.

## 3. Step-by-Step Porting Plan

This plan proceeds from foundational utilities to complex processing objects.

### Phase 1: Core Utilities & Data Structures

1.  **Constants:** Create `Source/dsp/Constants.h`. Move global constants like `kSqrtTwo` into this file as `constexpr` variables within the `IIRFilters::DSP` namespace.
2.  **Utility Functions:** Create `Source/dsp/Utilities.h` and `.cpp`. Port the global helper functions (`doLinearInterpolation`, `raw2dB`, `sgn`, etc.). Review each one and replace it with a JUCE equivalent if one exists (e.g., `juce::Decibels::gainToDecibels`).
3.  **Core Structs:** Port simple data structures like `ComplexNumber` and `SignalGenData` into their own headers within `Source/dsp/`.

### Phase 2: Porting the Biquad Filter

The `Biquad` class is a fundamental building block for many other objects.

1.  **Create Files:** `Source/dsp/Biquad.h` and `Source/dsp/Biquad.cpp`.
2.  **Refactor Class:**
    *   Copy the `Biquad` class definition into the new header.
    *   Place it inside the `IIRFilters::DSP` namespace.
    *   Change the `coeffArray` and `stateArray` from raw arrays to `std::array<double, numCoeffs>` and `std::array<double, numStates>`.
    *   Move the implementation of `processAudioSample` and other methods to the `.cpp` file.
    *   The `reset` method will remain, but the `IAudioSignalProcessor` inheritance will be removed.
3.  **Unit Test:** Create a JUCE unit test for `Biquad`. The test should:
    *   Instantiate the biquad.
    *   Set known coefficients (e.g., for a simple low-pass filter).
    *   Process a known signal (e.g., an impulse).
    *   Assert that the output matches the expected output from the original implementation.

### Phase 3: Porting the AudioFilter Class

1.  **Create Files:** `Source/dsp/AudioFilter.h` and `Source/dsp/AudioFilter.cpp`.
2.  **Refactor Class:**
    *   This class is a wrapper around `Biquad`. The porting process is similar.
    *   It will contain a `IIRFilters::DSP::Biquad` member.
    *   The `calculateFilterCoeffs` method will be the most complex part. It should be ported carefully.
    *   The `AudioFilterParameters` struct will be kept for configuring the filter's behavior internally, but it will not be used for the plugin's public parameter interface.
3.  **Unit Test:** Create a comprehensive unit test for `AudioFilter`. Test each `filterAlgorithm` type to ensure the coefficient calculations are correct and the audio output is as expected.

### Phase 4: Porting Remaining FX Objects

Proceed through the rest of the classes in `fxobjects.h` one by one, following the same pattern:

1.  **Create Files** for the object (e.g., `AudioDetector.h`, `DynamicsProcessor.h`).
2.  **Refactor** the class using modern C++ and JUCE idioms.
3.  **Create a Unit Test** to verify its behavior against the original.

**Priority List for Porting:**
1.  `AudioDetector`
2.  `DynamicsProcessor`
3.  `LRFilterBank`
4.  Other objects as needed.

### Phase 5: JUCE Plugin Integration

1.  **Parameters:** In `PluginProcessor.h`, define all user-facing parameters using `juce::AudioProcessorValueTreeState`. This will include things like filter frequency, Q, ratio, threshold, etc.
2.  **Instantiate Objects:** In `PluginProcessor.h`, create instances of the newly ported DSP objects (e.g., `IIRFilters::DSP::AudioFilter`).
3.  **Process Block:** In `PluginProcessor::processBlock`, do the following:
    *   Get the latest parameter values from the `AudioProcessorValueTreeState`.
    *   Update the DSP objects with these new values (e.g., call `audioFilter.setParameters(...)`).
    *   Process the audio from the `juce::AudioBuffer` through the DSP objects.
4.  **UI:** Create a basic UI in `PluginEditor` with sliders and buttons attached to the `AudioProcessorValueTreeState` to control the plugin.

## 4. Verification

Throughout the process, unit tests are the primary method of verification. Once the plugin is assembled, further testing will involve:
*   Loading the plugin in a DAW.
*   Comparing its sound and behavior to the original VST/AU plugin.
*   Using analysis tools (e.g., Plugin Doctor) to compare frequency responses, gain reduction curves, etc.

---

## 5. Example: Porting Biquad as a Framework-Agnostic Class

This example demonstrates how to port the `Biquad` class so it can be reused in any framework (JUCE, iPlug2, etc.) with minimal effort. The key is to ensure the DSP class itself has **no dependencies on any framework-specific code**.

### The Platform-Agnostic Header: `Source/dsp/Biquad.h`

This header only includes standard C++ libraries. It defines the class interface, using `std::array` for fixed-size arrays and clear, modern C++ syntax.

```cpp
#pragma once

#include <array>
#include <cmath> // For std::memset

namespace IIRFilters
{
namespace DSP
{
    // --- From fxobjects.h
    enum FilterCoeffs { a0, a1, a2, b1, b2, c0, d0, NumCoeffs };
    enum StateReg { x_z1, x_z2, y_z1, y_z2, NumStates };
    enum class BiquadAlgorithm { kDirect, kCanonical, kTransposeDirect, kTransposeCanonical };

    class Biquad
    {
    public:
        Biquad() = default;
        ~Biquad() = default;

        /** Clears the state registers. */
        void reset()
        {
            // Using std::memset for efficiency, equivalent to looping and setting to 0.0
            std::memset(state.data(), 0, sizeof(double) * NumStates);
        }

        /** Processes a single audio sample.
            @param inputSample the input sample to process
            @return the processed output sample
        */
        double processAudioSample(double inputSample);

        /** Sets the filter coefficients from a C-style array.
            @param newCoeffs pointer to an array of doubles with `NumCoeffs` elements
        */
        void setCoefficients(const double* newCoeffs)
        {
            std::memcpy(coeffs.data(), newCoeffs, sizeof(double) * NumCoeffs);
        }

        /** Sets the biquad calculation algorithm to use. */
        void setAlgorithm(BiquadAlgorithm newAlgorithm)
        {
            algorithm = newAlgorithm;
        }

    private:
        std::array<double, NumCoeffs> coeffs{};
        std::array<double, NumStates> state{};
        BiquadAlgorithm algorithm = BiquadAlgorithm::kDirect;
    };

} // namespace DSP
} // namespace IIRFilters
```

### The Implementation File: `Source/dsp/Biquad.cpp`

This file contains the actual DSP logic. It only includes its own header. The `processAudioSample` function is a direct port of the original logic, adapted for the `std::array` members.

```cpp
#include "Biquad.h"

namespace IIRFilters
{
namespace DSP
{
    double Biquad::processAudioSample(double inputSample)
    {
        // --- Alias for readability
        const auto& c = coeffs;
        auto& s = state;

        if (algorithm == BiquadAlgorithm::kDirect)
        {
            double yn = c[a0] * inputSample + c[a1] * s[x_z1] + c[a2] * s[x_z2] - c[b1] * s[y_z1] - c[b2] * s[y_z2];
            s[x_z2] = s[x_z1];
            s[x_z1] = inputSample;
            s[y_z2] = s[y_z1];
            s[y_z1] = yn;
            return yn;
        }

        if (algorithm == BiquadAlgorithm::kCanonical)
        {
            double wn = inputSample - c[b1] * s[x_z1] - c[b2] * s[x_z2];
            double yn = c[a0] * wn + c[a1] * s[x_z1] + c[a2] * s[x_z2];
            s[x_z2] = s[x_z1];
            s[x_z1] = wn;
            return yn;
        }

        if (algorithm == BiquadAlgorithm::kTransposeDirect)
        {
            double wn = inputSample + s[x_z1];
            double yn = c[a0] * wn + s[y_z1];
            s[x_z1] = c[a1] * wn - c[b1] * yn + s[x_z2];
            s[x_z2] = c[a2] * wn - c[b2] * yn;
            return yn;
        }

        if (algorithm == BiquadAlgorithm::kTransposeCanonical)
        {
            double yn = c[a0] * inputSample + s[x_z1];
            s[x_z1] = c[a1] * inputSample - c[b1] * yn + s[x_z2];
            s[x_z2] = c[a2] * inputSample - c[b2] * yn;
            return yn;
        }

        return inputSample; // Should not happen
    }

} // namespace DSP
} // namespace IIRFilters
```

### How to Use in a Framework (The "Glue" Code)

Your JUCE `PluginProcessor` (or iPlug2 `IPlugAPIBase` derived class) is responsible for connecting the framework to your agnostic DSP code.

**Example: JUCE `PluginProcessor.cpp`**

```cpp
#include "PluginProcessor.h"
#include "dsp/Biquad.h" // Include your agnostic DSP class

// ... other includes

class MyPluginAudioProcessor : public juce::AudioProcessor
{
    // ... other processor code

private:
    // --- Create an instance of your DSP object
    IIRFilters::DSP::Biquad biquadFilter;

    // --- JUCE parameters (e.g., from AudioProcessorValueTreeState)
    // juce::AudioParameterFloat* cutoffParam;
    // juce::AudioParameterFloat* qParam;
};

void MyPluginAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // --- Reset the biquad state when playback starts
    biquadFilter.reset();

    // --- Set an initial algorithm
    biquadFilter.setAlgorithm(IIRFilters::DSP::BiquadAlgorithm::kTransposeCanonical);
}

void MyPluginAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    // 1. GET PARAMETERS from framework (e.g., APVTS)
    // double cutoff = cutoffParam->get();
    // double q = qParam->get();

    // 2. CALCULATE COEFFICIENTS (using a separate, agnostic calculator class)
    // double coeffs[IIRFilters::DSP::NumCoeffs];
    // calculateMyLowpassCoeffs(coeffs, cutoff, q, getSampleRate());

    // 3. UPDATE DSP OBJECT
    // biquadFilter.setCoefficients(coeffs);

    // 4. PROCESS AUDIO
    // This is the "glue" part. You iterate the framework's audio buffer
    // and pass samples to your agnostic processAudioSample() method.
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            channelData[i] = biquadFilter.processAudioSample(channelData[i]);
        }
    }
}
```

By following this pattern, the `IIRFilters::DSP::Biquad` class can be copied and used in any project without changing a single line of its code. All the framework-specific logic is contained in the `PluginProcessor` which acts as the adapter.

## 6. Example: Parameter Handling for `AudioFilter`

This follows the same principle: the DSP class is stripped of framework or parameter-handling logic and is instead controlled by simple public methods. The `PluginProcessor` is responsible for translating UI/host parameter changes into calls to these methods.

This decouples the DSP from the parameter management system, making it fully reusable.

### The Agnostic `AudioFilter.h` (No Parameter Struct)

Notice the `AudioFilterParameters` struct is gone. It's replaced by public setter methods. The class now stores its parameters as simple private members.

```cpp
// In: Source/dsp/AudioFilter.h

#pragma once

#include "Biquad.h"

namespace IIRFilters
{
namespace DSP
{
    // Original enum, still useful and self-contained
    enum class FilterAlgorithm { kLPF1, kHPF1, kLPF2, /* ...etc */ };

    class AudioFilter
    {
    public:
        AudioFilter() = default;

        // --- Configuration ---
        void prepare(double newSampleRate);
        void reset() { biquad.reset(); }

        // --- Parameter Setters ---
        void setAlgorithm(FilterAlgorithm newAlgorithm);
        void setCutoffFrequency(double newFreq);
        void setQ(double newQ);
        void setGain(double newGainDb);

        // --- Audio Processing ---
        double processAudioSample(double inputSample)
        {
            return biquad.processAudioSample(inputSample);
        }

    private:
        // This becomes the core logic, called whenever a parameter changes
        void recalculateCoefficients();

        Biquad biquad;
        double sampleRate = 44100.0;

        // Internal state for parameters, controlled by public setters
        FilterAlgorithm currentAlgorithm = FilterAlgorithm::kLPF2;
        double cutoffFrequency = 1000.0;
        double q = 0.707;
        double gainDb = 0.0;
    };

} // namespace DSP
} // namespace IIRFilters
```

### The `PluginProcessor` (The "Glue" Code)

The `PluginProcessor` owns the `AudioFilter` instance and is responsible for feeding it parameter values from JUCE's state manager during the process block.

```cpp
// In: Source/PluginProcessor.cpp (Simplified)

#include "PluginProcessor.h"
#include "dsp/AudioFilter.h" // Your agnostic class

// ...

void MyPluginAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Pass the sample rate to the DSP object
    filter.prepare(sampleRate);
}

void MyPluginAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    // 1. Read current parameter values from the thread-safe APVTS
    auto newFreq = apvts.getRawParameterValue("CUTOFF")->load();
    auto newQ = apvts.getRawParameterValue("Q")->load();
    auto newGain = apvts.getRawParameterValue("GAIN")->load();
    auto newAlgoIndex = static_cast<int>(apvts.getRawParameterValue("ALGORITHM")->load());

    // 2. Update the agnostic DSP object with the new values.
    //    This is the crucial "glue" step.
    filter.setCutoffFrequency(newFreq);
    filter.setQ(newQ);
    filter.setGain(newGain);
    filter.setAlgorithm(static_cast<IIRFilters::DSP::FilterAlgorithm>(newAlgoIndex));

    // 3. Process the audio using the now-configured DSP object
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            // Cast to double for processing, then back to float for the buffer
            channelData[i] = static_cast<float>(filter.processAudioSample(static_cast<double>(channelData[i])));
        }
    }
}

// ... In PluginProcessor.h
// private:
//     juce::AudioProcessorValueTreeState apvts;
//     IIRFilters::DSP::AudioFilter filter;
```

By adopting this architecture, your core `AudioFilter` logic remains clean, portable, and easy to unit-test, while the `PluginProcessor` handles all the framework-specific responsibilities.