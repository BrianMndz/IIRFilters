# Example Audio Plugin CMakeLists.txt
cmake_minimum_required(VERSION 3.24)

set(PROJECT_NAME MyAwesomePlugin)

# Change here the project name
project(${PROJECT_NAME} VERSION 0.0.1)

set (CMAKE_CXX_STANDARD 17)
set (COMPANY_NAME "Company Name")
set (CMAKE_XCODE_GENERATE_SCHEME OFF)
set (CMAKE_EXPORT_COMPILE_COMMANDS on)
set (CMAKE_OSX_ARCHITECTURES "x86_64;arm64")
set (CMAKE_OSX_DEPLOYMENT_TARGET "10.15")

add_subdirectory(Modules/JUCE)                    # If you've put JUCE in a subdirectory called JUCE

# If you are building a VST2 or AAX plugin, CMake needs to be told where to find these SDKs on your
# system. This setup should be done before calling `juce_add_plugin`.

set_property(GLOBAL PROPERTY USE_FOLDERS YES)
option(JUCE_ENABLE_MODULE_SOURCE_GROUPS "Show all module sources in IDE projects" ON)

# juce_set_vst2_sdk_path(...)
# juce_set_aax_sdk_path(...)

# `target_sources` adds source files to a target. We pass the target that needs the sources as the
# first argument, then a visibility parameter for the sources which should normally be PRIVATE.
# Finally, we supply a list of source files that will be built into the target. This is a standard
# CMake command.

# Source Files
macro (setSourceFiles sourceFiles)
    file (GLOB_RECURSE SOURCES
            "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    )
    set (${sourceFiles} ${SOURCES})
endmacro()

add_subdirectory(Source)

# target_sources(${PROJECT_NAME}
#    PRIVATE
#        Source/PluginEditor.cpp
#        Source/PluginProcessor.cpp)

# `target_compile_definitions` adds some preprocessor definitions to our target. In a Projucer
# project, these might be passed in the 'Preprocessor Definitions' field. JUCE modules also make use
# of compile definitions to switch certain features on/off, so if there's a particular feature you
# need that's not on by default, check the module header for the correct flag to set here. These
# definitions will be visible both to your code, and also the JUCE module code, so for new
# definitions, pick unique names that are unlikely to collide! This is a standard CMake command.



# If your target needs extra binary assets, you can add them here. The first argument is the name of
# a new static library target that will include all the binary resources. There is an optional
# `NAMESPACE` argument that can specify the namespace of the generated binary data class. Finally,
# the SOURCES argument should be followed by a list of source files that should be built into the
# static library. These source files can be of any kind (wav data, images, fonts, icons etc.).
# Conversion to binary-data will happen when your target is built.

# juce_add_binary_data(AudioPluginData SOURCES ...)

# `target_link_libraries` links libraries and JUCE modules to other libraries or executables. Here,
# we're linking our executable target to the `juce::juce_audio_utils` module. Inter-module
# dependencies are resolved automatically, so `juce_core`, `juce_events` and so on will also be
# linked automatically. If we'd generated a binary data target above, we would need to link to it
# here too. This is a standard CMake command.



