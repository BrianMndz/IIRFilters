#include "PluginProcessor.h"
#include "PluginEditor.h"

static auto streamToVector (InputStream& stream)
{
    std::vector<std::byte> result ((size_t) stream.getTotalLength());
    stream.setPosition (0);
    [[maybe_unused]] const auto bytesRead = stream.read (result.data(), result.size());
    jassert (bytesRead == (ssize_t) result.size());
    return result;
}

static const char* getMimeForExtension (const juce::String& extension)
{
    static const std::unordered_map<String, const char*> mimeMap =
    {
        { { "htm"   },  "text/html"                },
        { { "html"  },  "text/html"                },
        { { "txt"   },  "text/plain"               },
        { { "jpg"   },  "image/jpeg"               },
        { { "jpeg"  },  "image/jpeg"               },
        { { "svg"   },  "image/svg+xml"            },
        { { "ico"   },  "image/vnd.microsoft.icon" },
        { { "json"  },  "application/json"         },
        { { "png"   },  "image/png"                },
        { { "css"   },  "text/css"                 },
        { { "map"   },  "application/json"         },
        { { "js"    },  "text/javascript"          },
        { { "woff2" },  "font/woff2"               }
    };

    if (const auto it = mimeMap.find (extension.toLowerCase()); it != mimeMap.end())
        return it->second;

    jassertfalse;
    return "";
}

//==============================================================================
IIRFiltersAudioProcessorEditor::IIRFiltersAudioProcessorEditor (IIRFiltersAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p),
    webView{juce::WebBrowserComponent::Options{}.withBackend(
        juce::WebBrowserComponent::Options::Backend::webview2)
        .withWinWebView2Options(juce::WebBrowserComponent::Options::WinWebView2{}
            .withUserDataFolder(juce::File::getSpecialLocation(juce::File::tempDirectory)))
            .withResourceProvider(
                [this](const auto &url) { return getResource(url);})}
{
    juce::ignoreUnused(processorRef);

    addAndMakeVisible(webView);

    webView.goToURL(webView.getResourceProviderRoot());

    setResizable(true, true);
    setSize (800, 600);
}

IIRFiltersAudioProcessorEditor::~IIRFiltersAudioProcessorEditor() = default;

void IIRFiltersAudioProcessorEditor::resized()
{
    webView.setBounds (getLocalBounds());
}

auto IIRFiltersAudioProcessorEditor::getResource(const juce::String& url) -> std::optional<Resource>
{
    std::cout << url << std::endl;

    static const auto resourceFileRoot = juce::File{"/Users/brianmendoza/Development/audio/IIRFilters/Source/gui"};

    const auto resourceToRetrieve = url == "/" ? "index.html" :
        url.fromFirstOccurrenceOf("/", false, false);

    const auto resource = resourceFileRoot.getChildFile(resourceToRetrieve).createInputStream();

    if (resource) {
        const auto extension = resourceToRetrieve.fromFirstOccurrenceOf(".", false, false);
        return Resource{streamToVector(*resource), getMimeForExtension(extension)};
    }

    return std::nullopt;
}