#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
// Simple fallback editor - currently unused since we use GenericAudioProcessorEditor
IIRFiltersAudioProcessorEditor::IIRFiltersAudioProcessorEditor (IIRFiltersAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p), webBrowserComponent(juce::WebBrowserComponent::Options{})
{
    // For development, we'll load the HTML file directly from the source directory.
    // IMPORTANT: This path is for development only. For a release build, you would
    // embed the GUI files into the binary using juce_add_binary_data.
    auto relativePath = juce::File::getSpecialLocation(juce::File::currentExecutableFile)
                            .getParentDirectory()
                            .getParentDirectory()
                            .getChildFile("Source")
                            .getChildFile("gui")
                            .getChildFile("index.html");

    // if (relativePath.existsAsFile())
    // {
    //     webBrowserComponent.goToURL(relativePath.getFullPathName());
    // }
    // else
    // {
    //     // Fallback if the file isn't found
    //     webBrowserComponent.goToURL("about:blank");
    // }

    addAndMakeVisible(webBrowserComponent);

    webBrowserComponent.goToURL("https://juce.com");

    setResizable(true, true);
    setSize (800, 600);
}

IIRFiltersAudioProcessorEditor::~IIRFiltersAudioProcessorEditor() = default;

//==============================================================================
// void IIRFiltersAudioProcessorEditor::paint (juce::Graphics& g)
// {
//     g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
// }

void IIRFiltersAudioProcessorEditor::resized()
{
    webBrowserComponent.setBounds (getLocalBounds());
}