#include "VstRack.h"
#include "GuiTheme.h"

VstRack::VstRack()
{
    formatManager.addDefaultFormats();
    resetDefaultSearchPaths();
}

VstRack::~VstRack()
{
    closeAllEditors();
    std::lock_guard<std::mutex> lock(rackMutex);
    slots.clear();
}

void VstRack::prepare(double sampleRate, int samplesPerBlock)
{
    std::lock_guard<std::mutex> lock(rackMutex);
    currentSampleRate = sampleRate;
    currentBlockSize = samplesPerBlock;

    for (auto& slot : slots)
    {
        if (slot && slot->plugin)
        {
            slot->plugin->setPlayConfigDetails(2, 2, sampleRate, samplesPerBlock);
            slot->plugin->prepareToPlay(sampleRate, samplesPerBlock);
        }
    }
}

void VstRack::release()
{
    std::lock_guard<std::mutex> lock(rackMutex);
    for (auto& slot : slots)
    {
        if (slot && slot->plugin)
            slot->plugin->releaseResources();
    }
}

void VstRack::process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    std::lock_guard<std::mutex> lock(rackMutex);

    for (auto& slot : slots)
    {
        if (slot && slot->plugin && slot->enabled && !slot->bypassed)
        {
            // Safeguard against buggy plugin crashes
            try
            {
                slot->plugin->processBlock(buffer, midi);
            }
            catch (...)
            {
                // Auto bypass crashed plugin to prevent application crash
                slot->bypassed = true;
            }
        }
    }
}

bool VstRack::loadPlugin(const juce::File& file, juce::String& errorMessage)
{
    if (!file.exists())
    {
        errorMessage = "File VST3 không tồn tại: " + file.getFullPathName();
        return false;
    }

    juce::OwnedArray<juce::PluginDescription> descriptions;
    juce::KnownPluginList pluginList;

    for (int i = 0; i < formatManager.getNumFormats(); ++i)
    {
        auto* format = formatManager.getFormat(i);
        format->findAllTypesForFile(descriptions, file.getFullPathName());
    }

    if (descriptions.isEmpty())
    {
        errorMessage = "Không tìm thấy định dạng VST3 tương thích trong file này.";
        return false;
    }

    auto desc = *descriptions[0];
    auto pluginInstance = formatManager.createPluginInstance(desc, currentSampleRate, currentBlockSize, errorMessage);

    if (pluginInstance == nullptr)
    {
        if (errorMessage.isEmpty())
            errorMessage = "Không thể khởi tạo VST3 plugin instance.";
        return false;
    }

    pluginInstance->setPlayConfigDetails(2, 2, currentSampleRate, currentBlockSize);
    pluginInstance->prepareToPlay(currentSampleRate, currentBlockSize);

    auto newSlot = std::make_unique<VstSlot>();
    newSlot->name = pluginInstance->getName();
    newSlot->fileLocation = file;
    newSlot->description = desc;
    newSlot->plugin = std::move(pluginInstance);
    newSlot->enabled = true;
    newSlot->bypassed = false;

    {
        std::lock_guard<std::mutex> lock(rackMutex);
        slots.push_back(std::move(newSlot));
    }

    notifyListeners();
    return true;
}

void VstRack::removePlugin(int index)
{
    {
        std::lock_guard<std::mutex> lock(rackMutex);
        if (index >= 0 && index < (int)slots.size())
        {
            if (slots[index]->editorWindow != nullptr)
                slots[index]->editorWindow->setVisible(false);
            slots.erase(slots.begin() + index);
        }
    }
    notifyListeners();
}

void VstRack::moveUp(int index)
{
    if (index > 0 && index < (int)slots.size())
    {
        std::lock_guard<std::mutex> lock(rackMutex);
        std::swap(slots[index], slots[index - 1]);
    }
    notifyListeners();
}

void VstRack::moveDown(int index)
{
    if (index >= 0 && index < (int)slots.size() - 1)
    {
        std::lock_guard<std::mutex> lock(rackMutex);
        std::swap(slots[index], slots[index + 1]);
    }
    notifyListeners();
}

void VstRack::setBypass(int index, bool bypass)
{
    std::lock_guard<std::mutex> lock(rackMutex);
    if (index >= 0 && index < (int)slots.size() && slots[index])
    {
        slots[index]->bypassed = bypass;
    }
}

void VstRack::setEnabled(int index, bool enable)
{
    std::lock_guard<std::mutex> lock(rackMutex);
    if (index >= 0 && index < (int)slots.size() && slots[index])
    {
        slots[index]->enabled = enable;
    }
}

void VstRack::openEditor(int index)
{
    if (index < 0 || index >= (int)slots.size())
        return;

    auto& slot = slots[index];
    if (slot && slot->plugin)
    {
        if (slot->editorWindow == nullptr)
        {
            slot->editorWindow = std::make_unique<PluginWindow>(
                slot->plugin.get(),
                slot->name + " - HNStudio VST Host",
                HNStudioTheme::bgDark
            );
        }
        slot->editorWindow->centreWithSize(600, 450);
        slot->editorWindow->setVisible(true);
        slot->editorWindow->toFront(true);
    }
}

void VstRack::closeAllEditors()
{
    for (auto& slot : slots)
    {
        if (slot && slot->editorWindow != nullptr)
            slot->editorWindow->setVisible(false);
    }
}

int VstRack::getNumPlugins() const
{
    return (int)slots.size();
}

const VstSlot* VstRack::getSlot(int index) const
{
    if (index >= 0 && index < (int)slots.size())
        return slots[index].get();
    return nullptr;
}

std::vector<VstRack::KeySyncResult> VstRack::sendKeyToPlugins(int keyIndex, bool isMajor)
{
    std::vector<KeySyncResult> results;
    std::lock_guard<std::mutex> lock(rackMutex);

    static const char* keyNames[] = {
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
    };
    juce::String targetKey = (keyIndex >= 0 && keyIndex < 12) ? keyNames[keyIndex] : "C";
    juce::String targetScale = isMajor ? "Major" : "Minor";

    for (auto& slot : slots)
    {
        if (!slot || !slot->plugin)
            continue;

        KeySyncResult res;
        res.pluginName = slot->name;
        res.success = false;

        auto* proc = slot->plugin.get();
        bool foundKeyParam = false;

        for (int p = 0; p < proc->getNumParameters(); ++p)
        {
            auto pName = proc->getParameterName(p).toLowerCase();
            if (pName.contains("key") || pName.contains("root") || pName.contains("tonic"))
            {
                // Normalize 0.0 to 1.0
                float normVal = (float)keyIndex / 11.0f;
                proc->setParameter(p, normVal);
                foundKeyParam = true;
            }
            if (pName.contains("scale") || pName.contains("mode"))
            {
                proc->setParameter(p, isMajor ? 0.0f : 1.0f);
            }
        }

        if (foundKeyParam)
        {
            res.success = true;
            res.message = "Đã đồng bộ Key: " + targetKey + " " + targetScale + " vào " + slot->name;
        }
        else
        {
            res.success = false;
            res.message = "Plugin không hỗ trợ điều khiển Key tự động.";
        }
        results.push_back(res);
    }

    return results;
}

void VstRack::notifyListeners()
{
    listeners.call([](Listener& l) { l.vstRackChanged(); });
}

void VstRack::resetDefaultSearchPaths()
{
    searchPaths.clear();
   #if JUCE_WINDOWS
    searchPaths.add("C:\\Program Files\\Common Files\\VST3");
    searchPaths.add("C:\\Program Files\\VSTPlugins");
    searchPaths.add("C:\\Program Files\\Steinberg\\VstPlugins");
    searchPaths.add("C:\\Program Files\\Common Files\\VST2");
   #else
    searchPaths.add("/Library/Audio/Plug-Ins/VST3");
    searchPaths.add("~/Library/Audio/Plug-Ins/VST3");
   #endif
}

void VstRack::addSearchPath(const juce::String& path)
{
    if (path.isNotEmpty() && !searchPaths.contains(path))
    {
        searchPaths.add(path);
    }
}

void VstRack::removeSearchPath(int index)
{
    if (index >= 0 && index < searchPaths.size())
    {
        searchPaths.remove(index);
    }
}

const juce::StringArray& VstRack::getSearchPaths() const
{
    return searchPaths;
}

void VstRack::scanPlugins(std::function<void(float progress, const juce::String& name)> onProgress,
                          std::function<void(int count)> onComplete)
{
    discoveredPlugins.clear();
    std::vector<juce::File> foundFiles;

    for (const auto& pathStr : searchPaths)
    {
        juce::File dir(pathStr);
        if (dir.isDirectory())
        {
            auto files = dir.findChildFiles(juce::File::findFilesAndDirectories, true, "*.vst3");
            for (const auto& f : files)
            {
                if (f.getFileName().endsWithIgnoreCase(".vst3"))
                    foundFiles.push_back(f);
            }
        }
    }

    int total = (int)foundFiles.size();
    if (total == 0)
    {
        if (onComplete) onComplete(0);
        return;
    }

    for (int i = 0; i < total; ++i)
    {
        const auto& file = foundFiles[i];
        if (onProgress)
        {
            float prog = (float)(i + 1) / (float)total;
            onProgress(prog, file.getFileNameWithoutExtension());
        }

        juce::OwnedArray<juce::PluginDescription> descriptions;
        for (int fmt = 0; fmt < formatManager.getNumFormats(); ++fmt)
        {
            formatManager.getFormat(fmt)->findAllTypesForFile(descriptions, file.getFullPathName());
        }

        for (auto* d : descriptions)
        {
            ScannedPlugin sp;
            sp.name = d->name.isNotEmpty() ? d->name : file.getFileNameWithoutExtension();
            sp.vendor = d->manufacturerName.isNotEmpty() ? d->manufacturerName : "VST3 Vendor";
            sp.path = file.getFullPathName();
            sp.desc = *d;
            discoveredPlugins.push_back(sp);
        }
    }

    if (onComplete)
        onComplete((int)discoveredPlugins.size());
}

bool VstRack::addPluginByDescription(const juce::PluginDescription& desc, juce::String& errorMessage)
{
    auto pluginInstance = formatManager.createPluginInstance(desc, currentSampleRate, currentBlockSize, errorMessage);
    if (pluginInstance == nullptr)
    {
        if (errorMessage.isEmpty()) errorMessage = "Không thể khởi tạo plugin instance.";
        return false;
    }

    pluginInstance->setPlayConfigDetails(2, 2, currentSampleRate, currentBlockSize);
    pluginInstance->prepareToPlay(currentSampleRate, currentBlockSize);

    auto newSlot = std::make_unique<VstSlot>();
    newSlot->name = pluginInstance->getName();
    newSlot->fileLocation = juce::File(desc.fileOrIdentifier);
    newSlot->description = desc;
    newSlot->plugin = std::move(pluginInstance);
    newSlot->enabled = true;
    newSlot->bypassed = false;

    {
        std::lock_guard<std::mutex> lock(rackMutex);
        slots.push_back(std::move(newSlot));
    }

    notifyListeners();
    return true;
}
