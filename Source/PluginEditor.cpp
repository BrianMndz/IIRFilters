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
    // For development, try multiple possible paths
    juce::File htmlFile;
    
    // Method 1: Relative to executable
    auto executableFile = juce::File::getSpecialLocation(juce::File::currentExecutableFile);
    auto relativePath = executableFile
                            .getParentDirectory()
                            .getParentDirectory()
                            .getChildFile("Source")
                            .getChildFile("gui")
                            .getChildFile("index.html");

    DBG("Executable path: " + executableFile.getFullPathName());
    DBG("Looking for GUI at: " + relativePath.getFullPathName());
    
    if (relativePath.existsAsFile())
    {
        htmlFile = relativePath;
    }
    else
    {
        // Method 2: Try going up one more level
        auto altPath = executableFile
                          .getParentDirectory()
                          .getParentDirectory()
                          .getParentDirectory()
                          .getChildFile("Source")
                          .getChildFile("gui")
                          .getChildFile("index.html");
        
        DBG("Trying alternative path 1: " + altPath.getFullPathName());
        
        if (altPath.existsAsFile())
        {
            htmlFile = altPath;
        }
        else
        {
            // Method 3: Direct absolute path (for development)
            auto directPath = juce::File("/Users/brianmendoza/Development/audio/IIRFilters/Source/gui/index.html");
            DBG("Trying direct path: " + directPath.getFullPathName());
            
            if (directPath.existsAsFile())
            {
                htmlFile = directPath;
            }
        }
    }

    if (htmlFile.existsAsFile())
    {
        DBG("Loading HTML from: " + htmlFile.getFullPathName());
        webBrowserComponent.goToURL("file://" + htmlFile.getFullPathName());
    }
    else
    {
        DBG("ERROR: Cannot find index.html file!");
        webBrowserComponent.goToURL("about:blank");
    }

    addAndMakeVisible(webBrowserComponent);

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