#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include <memory>
#include <mutex>
#include <functional>

class PluginWindow : public juce::DocumentWindow
{
public:
    PluginWindow(juce::AudioProcessor* processor, const juce::String& title, juce::Colour bg)
        : juce::DocumentWindow(title, bg, juce::DocumentWindow::closeButton)
    {
        setUsingNativeTitleBar(true);
        if (processor != nullptr && processor->hasEditor())
        {
            auto* editor = processor->createEditorIfNeeded();
            if (editor != nullptr)
            {
                setContentNonOwned(editor, true);
                setResizable(editor->isResizable(), false);
            }
        }
    }

    void closeButtonPressed() override
    {
        setVisible(false);
    }
};

struct VstSlot
{
    std::unique_ptr<juce::AudioPluginInstance> plugin;
    juce::PluginDescription description;
    juce::File fileLocation;
    juce::String name;
    bool enabled = true;
    bool bypassed = false;
    std::unique_ptr<PluginWindow> editorWindow;
};

class VstRack
{
public:
    VstRack();
    ~VstRack();

    void prepare(double sampleRate, int samplesPerBlock);
    void release();
    void process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi);

    bool loadPlugin(const juce::File& file, juce::String& errorMessage);
    void removePlugin(int index);
    void moveUp(int index);
    void moveDown(int index);
    void setBypass(int index, bool bypass);
    void setEnabled(int index, bool enable);
    void openEditor(int index);
    void closeAllEditors();

    int getNumPlugins() const;
    const VstSlot* getSlot(int index) const;

    // Auto-tune integration: attempts to sync key to plugins
    struct KeySyncResult
    {
        bool success = false;
        juce::String pluginName;
        juce::String message;
    };
    std::vector<KeySyncResult> sendKeyToPlugins(int keyIndex, bool isMajor);

    juce::AudioPluginFormatManager& getFormatManager() { return formatManager; }

    // VST Search & Scan paths
    void addSearchPath(const juce::String& path);
    void removeSearchPath(int index);
    const juce::StringArray& getSearchPaths() const;
    void resetDefaultSearchPaths();

    // Plugin scanner
    struct ScannedPlugin
    {
        juce::String name;
        juce::String vendor;
        juce::String path;
        juce::PluginDescription desc;
    };
    const std::vector<ScannedPlugin>& getDiscoveredPlugins() const { return discoveredPlugins; }
    void scanPlugins(std::function<void(float progress, const juce::String& name)> onProgress,
                     std::function<void(int count)> onComplete);
    bool addPluginByDescription(const juce::PluginDescription& desc, juce::String& errorMessage);

    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void vstRackChanged() = 0;
    };

    void addListener(Listener* l) { listeners.add(l); }
    void removeListener(Listener* l) { listeners.remove(l); }

private:
    void notifyListeners();

    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;

    juce::AudioPluginFormatManager formatManager;
    std::vector<std::unique_ptr<VstSlot>> slots;
    std::vector<ScannedPlugin> discoveredPlugins;
    juce::StringArray searchPaths;
    std::mutex rackMutex;

    juce::ListenerList<Listener> listeners;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VstRack)
};
