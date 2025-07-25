# Tutorial: Building a Web-Based GUI for a JUCE Plugin

Welcome to this guide on replacing a standard C++ JUCE GUI with one built using modern web technologies. This is a powerful technique that separates your user interface from your audio processing logic, allowing for faster UI development and the use of popular web frameworks.

**The Goal:** To learn how to create a web-based front-end for our C++ audio plugin. We will focus on understanding the core concepts of how the C++ and JavaScript worlds communicate within a JUCE plugin.

**The Technology:** We will use the **official JUCE 8 WebView support**. This lets us embed a native OS web browser component into our plugin editor. For the UI itself, we'll start with **plain HTML, CSS, and JavaScript** to build a solid foundation before moving to more complex frameworks.

---

## Table of Contents

*   **Part 1: The C++ Foundation - Setting Up the WebView**
    *   [Step 1.1: Enable the WebView Module in CMake](#step-11-enable-the-webview-module-in-cmake)
    *   [Step 1.2: Create a Home for Our Web Code](#step-12-create-a-home-for-our-web-code)
    *   [Step 1.3: Modify the PluginEditor Header](#step-13-modify-the-plugineditor-header)
    *   [Step 1.4: Modify the PluginEditor Source](#step-14-modify-the-plugineditor-source)

*   **Part 2: Two-Way Communication**
    *   [Step 2.1: C++ to JS - Listening for Parameter Changes](#step-21-c-to-js---listening-for-parameter-changes)
    *   [Step 2.2: JS to C++ - Listening for GUI Messages](#step-22-js-to-c---listening-for-gui-messages)
    *   [Step 2.3: Implementing the Communication Logic](#step-23-implementing-the-communication-logic)

*   **Part 3: Building the Front-End**
    *   [Step 3.1: The HTML Structure (`index.html`)](#step-31-the-html-structure-indexhtml)
    *   [Step 3.2: The JavaScript Logic (`main.js`)](#step-32-the-javascript-logic-mainjs)
    *   [Step 3.3: The CSS Styling (`style.css`)](#step-33-the-css-styling-stylecss)

---

## Part 1: The C++ Foundation - Setting Up the WebView

Before we can write any HTML, we need to prepare our C++ project to host a web view.

### Step 1.1: Enable the WebView Module in CMake

**Concept:** A "WebView" is a component provided by the operating system that can render web pages. To use it, we need to tell our build system, CMake, to include the necessary JUCE code and link against the required system libraries.

**Your Task:**

1.  **Open this file:** `Source/CMakeLists.txt`
2.  **Find and replace** the large block starting with `juce_add_plugin(...)` as detailed below.

    <details>
    <summary>Click to see the code to find and replace</summary>

    **Find this code block:**
    ```cmake
    juce_add_plugin(${PROJECT_NAME}
            # VERSION ...
            # ... (many lines) ...
            JUCE_USE_CURL=0     # If you remove this, add `NEEDS_CURL TRUE` to the `juce_add_plugin` call
            JUCE_VST3_CAN_REPLACE_VST2=0)
    ```

    **Replace it with this code block:**
    ```cmake
    juce_add_plugin(${PROJECT_NAME}
            # VERSION ...                               # Set this if the plugin version is different to the project version
            # ICON_BIG ...                              # ICON_* arguments specify a path to an image file to use as an icon for the Standalone
            # ICON_SMALL ...
            # COMPANY_NAME ...                          # Specify the name of the plugin's author
            # IS_SYNTH TRUE/FALSE                       # Is this a synth or an effect?
            # NEEDS_MIDI_INPUT TRUE/FALSE               # Does the plugin need midi input?
            # NEEDS_MIDI_OUTPUT TRUE/FALSE              # Does the plugin need midi output?
            # IS_MIDI_EFFECT TRUE/FALSE                 # Is this plugin a MIDI effect?
            # EDITOR_WANTS_KEYBOARD_FOCUS TRUE/FALSE    # Does the editor need keyboard focus?
            NEEDS_WEB_BROWSER TRUE
            NEEDS_CURL TRUE
            # COPY_PLUGIN_AFTER_BUILD TRUE/FALSE        # Should the plugin be installed to a default location after building?
            PLUGIN_MANUFACTURER_CODE DPrt               # A four-character manufacturer id with at least one upper-case character
            PLUGIN_CODE iirR                            # A unique four-character plugin id with exactly one upper-case character
            # GarageBand 10.3 requires the first letter to be upper-case, and the remaining letters to be lower-case
            FORMATS AU VST3 Standalone                  # The formats to build. Other valid formats are: AAX Unity VST AU AUv3
            PRODUCT_NAME "IIRFilters")        # The name of the final executable, which can differ from the target name

    # Add all source files
    set(SourceFiles)
    setSourceFiles(SourceFiles)

    target_sources(${PROJECT_NAME} PRIVATE ${SourceFiles})
    set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "")
    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "" FILES ${SourceFiles})

    target_link_libraries(${PROJECT_NAME}
            PRIVATE
            # AudioPluginData           # If we'd created a binary data target, we'd link to it here
            juce::juce_audio_utils
            juce_audio_processors
            juce_dsp
            juce_gui_basics
            juce_gui_extra
            PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags
            juce::juce_recommended_warning_flags)

    target_compile_definitions(${PROJECT_NAME}
            PUBLIC
            JUCE_VST3_CAN_REPLACE_VST2=0)
    ```
    </details>

**Why:** We added `NEEDS_WEB_BROWSER TRUE` and `NEEDS_CURL TRUE` to the `juce_add_plugin` function. This is the modern JUCE way to declare that our plugin requires the web browser and networking capabilities. We also removed the old compile definitions that explicitly disabled them.

### Step 1.2: Create a Home for Our Web Code

**Concept:** It's good practice to keep our web files (HTML for structure, CSS for style, JavaScript for logic) organized and separate from our C++ source code.

**Your Task:**

1.  Create a new directory named `GUI` inside your `Source` directory.
2.  Inside `Source/GUI`, create three new, empty files:
    *   `index.html`
    *   `style.css`
    *   `main.js`

### Step 1.3: Modify the PluginEditor Header

**Concept:** The `PluginEditor.h` file defines the class for our UI window. We will change it to hold a `WebBrowserComponent` instead of any standard JUCE UI elements.

**Your Task:**

1.  **Open this file:** `Source/PluginEditor.h`
2.  **Replace the entire file content** with the code below.

    ```cpp
    #pragma once

    #include "PluginProcessor.h"
    #include <juce_gui_extra/juce_gui_extra.h>

    //==============================================================================
    class IIRFiltersAudioProcessorEditor : public juce::AudioProcessorEditor
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
    ```

**Why:** We've included the `juce_gui_extra` module header, which contains the `WebBrowserComponent`. We then declared a `webBrowserComponent` member variable, which will be the C++ object that hosts our web-based GUI.

### Step 1.4: Modify the PluginEditor Source

**Concept:** Now we need to implement the changes from the header. We will create the `WebBrowserComponent` and tell it to load our `index.html` file.

**Your Task:**

1.  **Open this file:** `Source/PluginEditor.cpp`
2.  **Replace the entire file content** with the code below.

    ```cpp
    #include "PluginProcessor.h"
    #include "PluginEditor.h"

    //==============================================================================
    IIRFiltersAudioProcessorEditor::IIRFiltersAudioProcessorEditor (IIRFiltersAudioProcessor& p)
        : AudioProcessorEditor (&p), processorRef (p)
    {
        // For development, we'll load the HTML file directly from the source directory.
        // IMPORTANT: This path is for development only. For a release build, you would
        // embed the GUI files into the binary using juce_add_binary_data.
        auto relativePath = juce::File::getSpecialLocation(juce::File::currentExecutableFile)
                                .getParentDirectory()
                                .getParentDirectory()
                                .getChildFile("Resources")
                                .getChildFile("GUI")
                                .getChildFile("index.html");

        if (relativePath.existsAsFile())
        {
            webBrowserComponent.goToURL(relativePath.getFullPathName());
        }
        else
        {
            // Fallback if the file isn't found
            webBrowserComponent.goToURL("about:blank");
        }

        addAndMakeVisible(webBrowserComponent);
        setSize (500, 400);
    }

    IIRFiltersAudioProcessorEditor::~IIRFiltersAudioProcessorEditor()
    {
    }

    //==============================================================================
    void IIRFiltersAudioProcessorEditor::paint (juce::Graphics& g)
    {
        // The web view will draw itself, but we can set a background color
        // in case it fails to load.
        g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
    }

    void IIRFiltersAudioProcessorEditor::resized()
    {
        // This is called when the editor is resized.
        // We'll make our web view fill the entire editor window.
        webBrowserComponent.setBounds (getLocalBounds());
    }
    ```
**Why:** We are telling the `webBrowserComponent` to load our `index.html` file. The path logic is a bit complex to locate the file during development. We then make the component visible and set its size to fill the entire editor window.

---

## Part 2: Two-Way Communication

This is the most critical part. We need to create a bridge so the C++ engine and JavaScript GUI can talk to each other.

### Step 2.1: C++ to JS - Listening for Parameter Changes

**Concept:** When a parameter changes in the C++ backend (e.g., from DAW automation), we need to notify the JavaScript GUI. We do this by making our `PluginEditor` a "listener" for parameter changes.

**Your Task:**

1.  **Open this file:** `Source/PluginEditor.h`
2.  **Modify the class definition** to inherit from `juce::AudioProcessorValueTreeState::Listener`.

    **Find this line:**
    ```cpp
    class IIRFiltersAudioProcessorEditor : public juce::AudioProcessorEditor
    ```
    **Replace it with this:**
    ```cpp
    class IIRFiltersAudioProcessorEditor : public juce::AudioProcessorEditor,
                                           public juce::AudioProcessorValueTreeState::Listener
    ```
3.  **Add the listener callback function** declaration inside the class:

    ```cpp
    public:
        // ... existing public members ...
        void parameterChanged (const juce::String& parameterID, float newValue) override;
    ```

**Why:** By inheriting from the `Listener` class and declaring the `parameterChanged` function, we are setting up our `PluginEditor` to be able to react to changes from the `AudioProcessorValueTreeState`.

### Step 2.2: JS to C++ - Listening for GUI Messages

**Concept:** When the user interacts with the web GUI, JavaScript needs to send a message to C++. We achieve this by making our `PluginEditor` a listener for messages coming from the `WebBrowserComponent`.

**Your Task:**

1.  **Open this file:** `Source/PluginEditor.h`
2.  **Modify the class definition** to also inherit from `juce::WebBrowserComponent::Listener`.

    **Find this line:**
    ```cpp
    class IIRFiltersAudioProcessorEditor : public juce::AudioProcessorEditor,
                                           public juce::AudioProcessorValueTreeState::Listener
    ```
    **Replace it with this:**
    ```cpp
    class IIRFiltersAudioProcessorEditor : public juce::AudioProcessorEditor,
                                           public juce::AudioProcessorValueTreeState::Listener,
                                           public juce::WebBrowserComponent::Listener
    ```
3.  **Add the listener callback function** declaration inside the class. The important one is `pageFinishedLoading`.

    ```cpp
    private:
        // ... existing private members ...
        void pageFinishedLoading (juce::WebBrowserComponent* browser, const juce::String& url) override;
        bool pageAboutToLoad (juce::WebBrowserComponent* browser, const juce::String& newUrl) override;
        void newDocumentReceived (juce::WebBrowserComponent* browser, const juce::String& message) override;
    ```

**Why:** By inheriting from this second `Listener` class, we can now implement functions that will be automatically called by the `WebBrowserComponent` when certain events happen, like the page finishing loading or JavaScript sending us a message.

### Step 2.3: Implementing the Communication Logic

**Concept:** Now we write the C++ code for the listener functions we just declared. This is the "glue" that connects the two worlds.

**Your Task:**

1.  **Open this file:** `Source/PluginEditor.cpp`
2.  **In the constructor**, register the editor as a listener for both the APVTS and the web browser.

    **Add these lines at the end of the constructor:**
    ```cpp
    // Register as a listener to the APVTS
    processorRef.getAPVTS()->addParameterListener("TYPE", this);
    processorRef.getAPVTS()->addParameterListener("CUTOFF", this);
    processorRef.getAPVTS()->addParameterListener("Q", this);
    processorRef.getAPVTS()->addParameterListener("GAIN", this);

    // Register as a listener to the web browser
    webBrowserComponent.addListener(this);
    ```
3.  **Add the implementation for the listener functions** to the `PluginEditor.cpp` file.

    ```cpp
    // C++ to JS: Called when a parameter changes in the backend
    void IIRFiltersAudioProcessorEditor::parameterChanged(const juce::String& parameterID, float newValue)
    {
        // Create a JavaScript command to call a function in our web GUI
        juce::String jsCommand = "window.updateGUI('" + parameterID + "', " + juce::String(newValue) + ");";
        webBrowserComponent.executeJavascript(jsCommand);
    }

    // JS to C++: Called when the web page sends a message
    void IIRFiltersAudioProcessorEditor::newDocumentReceived(juce::WebBrowserComponent* browser, const juce::String& message)
    {
        juce::StringArray tokens;
        tokens.addTokens(message, ":", "");

        if (tokens.size() == 2)
        {
            auto parameterID = tokens[0];
            auto value = tokens[1].getFloatValue();

            if (auto* parameter = processorRef.getAPVTS()->getParameter(parameterID))
            {
                parameter->setValueNotifyingHost(value);
            }
        }
    }
    
    // These are required by the listener, but we can leave them empty for now
    void IIRFiltersAudioProcessorEditor::pageFinishedLoading(juce::WebBrowserComponent* browser, const juce::String& url) {}
    bool IIRFiltersAudioProcessorEditor::pageAboutToLoad(juce::WebBrowserComponent* browser, const juce::String& newUrl) { return true; }
    ```

**Why:** In the constructor, we are actively telling the APVTS and the web browser to notify *this* object when things happen. The `parameterChanged` function sends data *out* to JavaScript. The `newDocumentReceived` function receives data *in* from JavaScript, parses the simple `param:value` message, and updates the C++ parameter state.

---

## Part 3: Building the Front-End

Now we can build the actual user interface with our web skills.

### Step 3.1: The HTML Structure (`index.html`)

**Concept:** HTML provides the skeleton of the page. We will use standard `<input type="range">` elements for our sliders.

**Your Task:**

1.  **Open this file:** `Source/GUI/index.html`
2.  **Add the following content:**

    ```html
    <!DOCTYPE html>
    <html lang="en">
    <head>
        <meta charset="UTF-8">
        <title>IIRFilters GUI</title>
        <link rel="stylesheet" href="style.css">
    </head>
    <body>
        <div class="container">
            <h1>IIR Filter</h1>
            <div class="param">
                <label for="CUTOFF">Cutoff</label>
                <input type="range" id="CUTOFF" class="slider" min="20" max="20000" step="1" value="1000">
                <span id="CUTOFF_label">1000 Hz</span>
            </div>
            <div class="param">
                <label for="Q">Q</label>
                <input type="range" id="Q" class="slider" min="0.1" max="18" step="0.01" value="0.707">
                <span id="Q_label">0.707</span>
            </div>
            <div class="param">
                <label for="GAIN">Gain</label>
                <input type="range" id="GAIN" class="slider" min="-24" max="24" step="0.1" value="0.0">
                <span id="GAIN_label">0.0 dB</span>
            </div>
        </div>
        <script src="main.js"></script>
    </body>
    </html>
    ```

**Why:** We've created a clean structure for our controls. Each slider has an `id` that **exactly matches** the parameter ID in our C++ `APVTS` (`CUTOFF`, `Q`, `GAIN`). This is crucial for communication.

### Step 3.2: The JavaScript Logic (`main.js`)

**Concept:** JavaScript brings the page to life. It will send updates to C++ when a slider moves, and it will listen for updates from C++.

**Your Task:**

1.  **Open this file:** `Source/GUI/main.js`
2.  **Add the following content:**

    ```javascript
    // This function sends a message to the C++ backend
    function sendToCpp(parameterID, value) {
        // The JUCE WebView provides this special function to send messages
        if (window.webkit && window.webkit.messageHandlers && window.webkit.messageHandlers.juce) {
            const message = `${parameterID}:${value}`;
            window.webkit.messageHandlers.juce.postMessage(message);
        } else {
            // You can add a fallback for testing in a normal browser here
            console.log(`Would send: ${parameterID}:${value}`);
        }
    }

    // This function is called by the C++ backend to update the GUI
    window.updateGUI = (parameterID, value) => {
        const slider = document.getElementById(parameterID);
        const label = document.getElementById(`${parameterID}_label`);
        if (slider) {
            slider.value = value;
        }
        if (label) {
            updateLabel(label, parameterID, value);
        }
    };
    
    function updateLabel(label, id, value) {
        if (id === 'CUTOFF') {
            label.textContent = `${parseFloat(value).toFixed(0)} Hz`;
        } else if (id === 'GAIN') {
            label.textContent = `${parseFloat(value).toFixed(1)} dB`;
        } else {
            label.textContent = parseFloat(value).toFixed(3);
        }
    }

    // Add event listeners to all sliders
    document.querySelectorAll('.slider').forEach(slider => {
        slider.addEventListener('input', (event) => {
            const id = event.target.id;
            const value = event.target.value;
            
            // Send the new value to C++
            sendToCpp(id, value);
            
            // Update the text label next to the slider
            const label = document.getElementById(`${id}_label`);
            if (label) {
                updateLabel(label, id, value);
            }
        });
    });
    ```

**Why:** We've created two main pathways: `sendToCpp` for JS-to-C++ communication, and `window.updateGUI` for C++-to-JS. We then loop through all our sliders and attach an event listener that calls these functions whenever the user interacts with them.

### Step 3.3: The CSS Styling (`style.css`)

**Concept:** CSS makes the UI look good. A well-styled UI is easier and more pleasant to use.

**Your Task:**

1.  **Open this file:** `Source/GUI/style.css`
2.  **Add the following content:**

    ```css
    body {
        font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
        background-color: #282c34;
        color: #abb2bf;
        margin: 0;
        padding: 20px;
        box-sizing: border-box;
        user-select: none;
    }

    .container {
        display: flex;
        flex-direction: column;
        gap: 15px;
    }

    h1 {
        color: #61afef;
        text-align: center;
        margin-bottom: 10px;
    }

    .param {
        display: grid;
        grid-template-columns: 50px 1fr 80px;
        align-items: center;
        gap: 10px;
    }

    label {
        text-align: right;
        font-size: 14px;
    }

    input[type="range"] {
        -webkit-appearance: none;
        width: 100%;
        height: 8px;
        background: #3a3f4b;
        border-radius: 5px;
        outline: none;
    }

    input[type="range"]::-webkit-slider-thumb {
        -webkit-appearance: none;
        appearance: none;
        width: 18px;
        height: 18px;
        background: #61afef;
        cursor: pointer;
        border-radius: 50%;
    }
    
    span {
        font-size: 12px;
        font-variant-numeric: tabular-nums;
    }
    ```

**Why:** This CSS provides a modern, dark-themed look for our plugin. It styles the sliders to be more visually appealing than the browser defaults and uses a grid layout to keep everything aligned neatly.

---

You now have the complete set of instructions. Follow them step-by-step, and you will have a fully functional web-based GUI for your JUCE plugin. Let me know if you have any questions at any point!