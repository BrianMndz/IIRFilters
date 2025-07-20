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
                       ),
       apvts(*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

AudioPluginAudioProcessor::~AudioPluginAudioProcessor()
{
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout AudioPluginAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    
    // Filter Type Parameter
    juce::StringArray filterTypeChoices = { 
        "LPF1P", "LPF1", "HPF1", "LPF2", "HPF2", "BPF2", "BSF2", 
        "ButterLPF2", "ButterHPF2", "ButterBPF2", "ButterBSF2", 
        "MMALPF2", "MMALPF2B", "LowShelf", "HiShelf", 
        "NCQParaEQ", "CQParaEQ", "LWRLPF2", "LWRHPF2",
        "APF1", "APF2", "ResonA", "ResonB", 
        "MatchLP2A", "MatchLP2B", "MatchBP2A", "MatchBP2B",
        "ImpInvLP1", "ImpInvLP2"
    };
    
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("TYPE", 1), "Filter Type", filterTypeChoices, 3));
    
    // Cutoff Frequency Parameter (20Hz to 20kHz, logarithmic scale)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("CUTOFF", 1), "Cutoff", 
        juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.25f), 1000.0f));
    
    // Q Parameter (0.1 to 18, linear scale)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("Q", 1), "Q", 
        juce::NormalisableRange<float>(0.1f, 18.0f, 0.01f), 0.707f));
    
    // Gain Parameter (-24dB to +24dB, linear scale)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("GAIN", 1), "Gain", 
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), 0.0f));
    
    return { params.begin(), params.end() };
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
    smoothedCutoff.setCurrentAndTargetValue(*apvts.getRawParameterValue("CUTOFF"));
    smoothedQ.setCurrentAndTargetValue(*apvts.getRawParameterValue("Q"));
    smoothedGain.setCurrentAndTargetValue(*apvts.getRawParameterValue("GAIN"));
    
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

    // Get current parameter values
    auto typeIndex = static_cast<int>(*apvts.getRawParameterValue("TYPE"));
    auto currentAlgorithm = static_cast<wpdsp::FilterAlgorithm>(typeIndex);
    
    // Check if algorithm changed to avoid unnecessary updates
    if (currentAlgorithm != lastAlgorithm)
    {
        leftFilter.setAlgorithm(currentAlgorithm);
        rightFilter.setAlgorithm(currentAlgorithm);
        lastAlgorithm = currentAlgorithm;
    }
    
    // Set smoothed parameter targets
    smoothedCutoff.setTargetValue(*apvts.getRawParameterValue("CUTOFF"));
    smoothedQ.setTargetValue(*apvts.getRawParameterValue("Q"));
    smoothedGain.setTargetValue(*apvts.getRawParameterValue("GAIN"));
    
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
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void AudioPluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioPluginAudioProcessor();
}
