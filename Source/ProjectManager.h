#pragma once
#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>

class AudioEngine;

class ProjectManager
{
public:
    ProjectManager(AudioEngine& engine);
    ~ProjectManager() = default;

    void newProject();
    bool saveProject(const juce::File& file);
    bool loadProject(const juce::File& file, juce::StringArray& missingPlugins);

    juce::File getCurrentProjectFile() const { return currentProjectFile; }
    bool hasFile() const { return currentProjectFile.existsAsFile(); }

private:
    AudioEngine& audioEngine;
    juce::File currentProjectFile;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProjectManager)
};
