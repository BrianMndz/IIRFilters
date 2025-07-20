#include "PluginProcessor.h"

//==============================================================================
AudioPluginAudioProcessor::AudioPluginAudioProcessor()
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
{
}

AudioPluginAudioProcessor::~AudioPluginAudioProcessor()
{
}

//==============================================================================
const juce::String AudioPluginAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AudioPluginAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool AudioPluginAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool AudioPluginAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double AudioPluginAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AudioPluginAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int AudioPluginAudioProcessor::getCurrentProgram()
{
    return 0;
}

void AudioPluginAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String AudioPluginAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void AudioPluginAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void AudioPluginAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Initialize DSP objects
    leftFilter.prepare(sampleRate);
    rightFilter.prepare(sampleRate);
    
    // Initialize parameter smoothing
    smoothedCutoff.reset(sampleRate, 0.01);  // 10ms smoothing
    smoothedQ.reset(sampleRate, 0.01);
    smoothedGain.reset(sampleRate, 0.01);
    
    // Set initial parameter values
    smoothedCutoff.setTargetValue(*apvts.getRawParameterValue(IIRFilters::cutoffFrequency));
    smoothedQ.setTargetValue(*apvts.getRawParameterValue(IIRFilters::qVal));
    smoothedGain.setTargetValue(*apvts.getRawParameterValue(IIRFilters::GainDb));
    
    juce::ignoreUnused(samplesPerBlock);
}

void AudioPluginAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool AudioPluginAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}

void AudioPluginAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);

    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear unused output channels
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    updateParameters();
    
    const int numSamples = buffer.getNumSamples();
    
    // Process each sample
    for (int i = 0; i < numSamples; ++i)
    {
        // Get the next smoothed values
        double newCutoff = smoothedCutoff.getNextValue();
        double newQ = smoothedQ.getNextValue();
        double newGain = smoothedGain.getNextValue();

        // Update the filter parameters
        leftFilter.setCutoff(newCutoff);
        leftFilter.setQ(newQ);
        leftFilter.setGainDb(newGain);

        rightFilter.setCutoff(newCutoff);
        rightFilter.setQ(newQ);
        rightFilter.setGainDb(newGain);
        
        // Process left channel (always exists)
        auto* leftData = buffer.getWritePointer(0);
        leftData[i] = static_cast<float>(
            leftFilter.processAudioSample(static_cast<double>(leftData[i]))
        );
        
        // Process right channel (if stereo)
        if (totalNumInputChannels > 1)
        {
            auto* rightData = buffer.getWritePointer(1);
            rightData[i] = static_cast<float>(
                rightFilter.processAudioSample(static_cast<double>(rightData[i]))
            );
        }
    }
}

void AudioPluginAudioProcessor::updateParameters()
{
    // Get current parameter values
    auto typeIndex = static_cast<int>(*apvts.getRawParameterValue(IIRFilters::filterType));
    auto currentAlgorithm = static_cast<wpdsp::FilterAlgorithm>(typeIndex);

    // Check if algorithm changed to avoid unnecessary updates
    if (currentAlgorithm != lastAlgorithm)
    {
        leftFilter.setAlgorithm(currentAlgorithm);
        rightFilter.setAlgorithm(currentAlgorithm);
        lastAlgorithm = currentAlgorithm;
    }

    // Set smoothed parameter targets
    smoothedCutoff.setTargetValue(*apvts.getRawParameterValue(IIRFilters::cutoffFrequency));
    smoothedQ.setTargetValue(*apvts.getRawParameterValue(IIRFilters::qVal));
    smoothedGain.setTargetValue(*apvts.getRawParameterValue(IIRFilters::GainDb));
}


//==============================================================================
bool AudioPluginAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* AudioPluginAudioProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor (*this);
}

//==============================================================================
void AudioPluginAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::MemoryOutputStream mos(destData, true);
    apvts.state.writeToStream(mos);
}

void AudioPluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto tree = juce::ValueTree::readFromData(data, static_cast<size_t>(sizeInBytes));
    if (tree.isValid())
    {
        apvts.replaceState(tree);
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioPluginAudioProcessor();
}
