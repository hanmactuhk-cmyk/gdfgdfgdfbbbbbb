#include "ProjectManager.h"
#include "AudioEngine.h"

ProjectManager::ProjectManager(AudioEngine& engine)
    : audioEngine(engine)
{
}

void ProjectManager::newProject()
{
    currentProjectFile = juce::File();
    audioEngine.setMicVolume(1.0f);
    audioEngine.setMusicVolume(0.85f);
    audioEngine.setMasterVolume(1.0f);

    auto& dsp = audioEngine.getDspChain();
    dsp.getNoiseGate().setEnabled(true);
    dsp.getNoiseGate().setThreshold(-45.0f);

    dsp.getCompressor().setEnabled(true);
    dsp.getCompressor().setThreshold(-18.0f);
    dsp.getCompressor().setRatio(3.5f);

    dsp.getEQ().setEnabled(true);
    for (int i = 0; i < ParametricEQ::NumBands; ++i)
    {
        dsp.getEQ().setBand(i, ParametricEQ::DefaultFrequencies[i], 0.0f, 1.0f, true);
    }

    dsp.getDeEsser().setEnabled(true);
    dsp.getDeEsser().setThreshold(-24.0f);

    dsp.getReverb().setEnabled(true);
    dsp.getReverb().applyPreset(ReverbProcessor::Preset::Vocal);

    dsp.getLimiter().setEnabled(true);
    dsp.getLimiter().setThreshold(-0.5f);

    audioEngine.getAutoKeyDetector().setAutoKeyEnabled(false);
    audioEngine.getAutoKeyDetector().setManualKey(0, true);
}

bool ProjectManager::saveProject(const juce::File& file)
{
    juce::DynamicObject::Ptr rootObj = new juce::DynamicObject();
    rootObj->setProperty("app", "HNStudio Musik AI");
    rootObj->setProperty("version", "1.0.0");
    rootObj->setProperty("author", "Hoài Nguyễn Studio (Zalo: 0965.043.000)");

    // Volumes
    juce::DynamicObject::Ptr volObj = new juce::DynamicObject();
    volObj->setProperty("mic", audioEngine.getMicVolume());
    volObj->setProperty("music", audioEngine.getMusicVolume());
    volObj->setProperty("master", audioEngine.getMasterVolume());
    rootObj->setProperty("volumes", volObj.get());

    // DSP
    auto& dsp = audioEngine.getDspChain();

    // Noise Gate
    juce::DynamicObject::Ptr ngObj = new juce::DynamicObject();
    ngObj->setProperty("enabled", dsp.getNoiseGate().isEnabled());
    ngObj->setProperty("threshold", dsp.getNoiseGate().getThreshold());
    ngObj->setProperty("attack", dsp.getNoiseGate().getAttack());
    ngObj->setProperty("release", dsp.getNoiseGate().getRelease());
    ngObj->setProperty("range", dsp.getNoiseGate().getRange());
    rootObj->setProperty("noiseGate", ngObj.get());

    // Compressor
    juce::DynamicObject::Ptr compObj = new juce::DynamicObject();
    compObj->setProperty("enabled", dsp.getCompressor().isEnabled());
    compObj->setProperty("threshold", dsp.getCompressor().getThreshold());
    compObj->setProperty("ratio", dsp.getCompressor().getRatio());
    compObj->setProperty("attack", dsp.getCompressor().getAttack());
    compObj->setProperty("release", dsp.getCompressor().getRelease());
    compObj->setProperty("makeup", dsp.getCompressor().getMakeupGain());
    rootObj->setProperty("compressor", compObj.get());

    // EQ
    juce::DynamicObject::Ptr eqObj = new juce::DynamicObject();
    eqObj->setProperty("enabled", dsp.getEQ().isEnabled());
    juce::Array<juce::var> bandsArray;
    for (int i = 0; i < ParametricEQ::NumBands; ++i)
    {
        auto b = dsp.getEQ().getBand(i);
        juce::DynamicObject::Ptr bandObj = new juce::DynamicObject();
        bandObj->setProperty("freq", b.frequency);
        bandObj->setProperty("gain", b.gainDb);
        bandObj->setProperty("q", b.q);
        bandObj->setProperty("enabled", b.enabled);
        bandsArray.add(bandObj.get());
    }
    eqObj->setProperty("bands", bandsArray);
    rootObj->setProperty("eq", eqObj.get());

    // DeEsser
    juce::DynamicObject::Ptr deEsserObj = new juce::DynamicObject();
    deEsserObj->setProperty("enabled", dsp.getDeEsser().isEnabled());
    deEsserObj->setProperty("freq", dsp.getDeEsser().getFrequency());
    deEsserObj->setProperty("threshold", dsp.getDeEsser().getThreshold());
    deEsserObj->setProperty("amount", dsp.getDeEsser().getAmount());
    rootObj->setProperty("deEsser", deEsserObj.get());

    // Reverb
    juce::DynamicObject::Ptr revObj = new juce::DynamicObject();
    revObj->setProperty("enabled", dsp.getReverb().isEnabled());
    revObj->setProperty("roomSize", dsp.getReverb().getRoomSize());
    revObj->setProperty("damping", dsp.getReverb().getDamping());
    revObj->setProperty("width", dsp.getReverb().getWidth());
    revObj->setProperty("wet", dsp.getReverb().getWet());
    revObj->setProperty("dry", dsp.getReverb().getDry());
    revObj->setProperty("preDelay", dsp.getReverb().getPreDelay());
    revObj->setProperty("preset", static_cast<int>(dsp.getReverb().getCurrentPreset()));
    rootObj->setProperty("reverb", revObj.get());

    // Limiter
    juce::DynamicObject::Ptr limObj = new juce::DynamicObject();
    limObj->setProperty("enabled", dsp.getLimiter().isEnabled());
    limObj->setProperty("threshold", dsp.getLimiter().getThreshold());
    limObj->setProperty("release", dsp.getLimiter().getRelease());
    limObj->setProperty("ceiling", dsp.getLimiter().getOutputCeiling());
    rootObj->setProperty("limiter", limObj.get());

    // Auto Key
    juce::DynamicObject::Ptr keyObj = new juce::DynamicObject();
    keyObj->setProperty("autoKeyEnabled", audioEngine.getAutoKeyDetector().isAutoKeyEnabled());
    keyObj->setProperty("keyIndex", audioEngine.getAutoKeyDetector().getCurrentKeyIndex());
    keyObj->setProperty("isMajor", audioEngine.getAutoKeyDetector().isCurrentMajor());
    rootObj->setProperty("autoKey", keyObj.get());

    // VST Plugins
    juce::Array<juce::var> vstArray;
    auto& vstRack = audioEngine.getVstRack();
    for (int i = 0; i < vstRack.getNumPlugins(); ++i)
    {
        const auto* slot = vstRack.getSlot(i);
        if (slot != nullptr)
        {
            juce::DynamicObject::Ptr slotObj = new juce::DynamicObject();
            slotObj->setProperty("name", slot->name);
            slotObj->setProperty("path", slot->fileLocation.getFullPathName());
            slotObj->setProperty("enabled", slot->enabled);
            slotObj->setProperty("bypassed", slot->bypassed);
            vstArray.add(slotObj.get());
        }
    }
    rootObj->setProperty("vstRack", vstArray);

    juce::var jsonVar(rootObj.get());
    juce::String jsonString = juce::JSON::toString(jsonVar, true);

    if (file.replaceWithText(jsonString))
    {
        currentProjectFile = file;
        return true;
    }
    return false;
}

bool ProjectManager::loadProject(const juce::File& file, juce::StringArray& missingPlugins)
{
    if (!file.existsAsFile())
        return false;

    juce::var parsedJson = juce::JSON::parse(file);
    if (!parsedJson.isObject())
        return false;

    // Restore Volumes
    if (parsedJson.hasProperty("volumes"))
    {
        auto vols = parsedJson["volumes"];
        if (vols.hasProperty("mic")) audioEngine.setMicVolume((float)vols["mic"]);
        if (vols.hasProperty("music")) audioEngine.setMusicVolume((float)vols["music"]);
        if (vols.hasProperty("master")) audioEngine.setMasterVolume((float)vols["master"]);
    }

    auto& dsp = audioEngine.getDspChain();

    // Noise Gate
    if (parsedJson.hasProperty("noiseGate"))
    {
        auto ng = parsedJson["noiseGate"];
        dsp.getNoiseGate().setEnabled((bool)ng["enabled"]);
        dsp.getNoiseGate().setThreshold((float)ng["threshold"]);
        dsp.getNoiseGate().setAttack((float)ng["attack"]);
        dsp.getNoiseGate().setRelease((float)ng["release"]);
        dsp.getNoiseGate().setRange((float)ng["range"]);
    }

    // Compressor
    if (parsedJson.hasProperty("compressor"))
    {
        auto comp = parsedJson["compressor"];
        dsp.getCompressor().setEnabled((bool)comp["enabled"]);
        dsp.getCompressor().setThreshold((float)comp["threshold"]);
        dsp.getCompressor().setRatio((float)comp["ratio"]);
        dsp.getCompressor().setAttack((float)comp["attack"]);
        dsp.getCompressor().setRelease((float)comp["release"]);
        dsp.getCompressor().setMakeupGain((float)comp["makeup"]);
    }

    // EQ
    if (parsedJson.hasProperty("eq"))
    {
        auto eq = parsedJson["eq"];
        dsp.getEQ().setEnabled((bool)eq["enabled"]);
        if (eq.hasProperty("bands") && eq["bands"].isArray())
        {
            auto* arr = eq["bands"].getArray();
            for (int i = 0; i < std::min((int)arr->size(), ParametricEQ::NumBands); ++i)
            {
                auto b = arr->getReference(i);
                dsp.getEQ().setBand(i, (float)b["freq"], (float)b["gain"], (float)b["q"], (bool)b["enabled"]);
            }
        }
    }

    // De-Esser
    if (parsedJson.hasProperty("deEsser"))
    {
        auto de = parsedJson["deEsser"];
        dsp.getDeEsser().setEnabled((bool)de["enabled"]);
        dsp.getDeEsser().setFrequency((float)de["freq"]);
        dsp.getDeEsser().setThreshold((float)de["threshold"]);
        dsp.getDeEsser().setAmount((float)de["amount"]);
    }

    // Reverb
    if (parsedJson.hasProperty("reverb"))
    {
        auto rev = parsedJson["reverb"];
        dsp.getReverb().setEnabled((bool)rev["enabled"]);
        dsp.getReverb().setRoomSize((float)rev["roomSize"]);
        dsp.getReverb().setDamping((float)rev["damping"]);
        dsp.getReverb().setWidth((float)rev["width"]);
        dsp.getReverb().setWet((float)rev["wet"]);
        dsp.getReverb().setDry((float)rev["dry"]);
        dsp.getReverb().setPreDelay((float)rev["preDelay"]);
    }

    // Limiter
    if (parsedJson.hasProperty("limiter"))
    {
        auto lim = parsedJson["limiter"];
        dsp.getLimiter().setEnabled((bool)lim["enabled"]);
        dsp.getLimiter().setThreshold((float)lim["threshold"]);
        dsp.getLimiter().setRelease((float)lim["release"]);
        dsp.getLimiter().setOutputCeiling((float)lim["ceiling"]);
    }

    // Auto Key
    if (parsedJson.hasProperty("autoKey"))
    {
        auto k = parsedJson["autoKey"];
        audioEngine.getAutoKeyDetector().setAutoKeyEnabled((bool)k["autoKeyEnabled"]);
        audioEngine.getAutoKeyDetector().setManualKey((int)k["keyIndex"], (bool)k["isMajor"]);
    }

    // VST Plugins
    if (parsedJson.hasProperty("vstRack") && parsedJson["vstRack"].isArray())
    {
        auto& vstRack = audioEngine.getVstRack();
        auto* vstArr = parsedJson["vstRack"].getArray();
        for (const auto& item : *vstArr)
        {
            juce::String path = item["path"].toString();
            juce::File vstFile(path);
            if (vstFile.existsAsFile())
            {
                juce::String err;
                if (vstRack.loadPlugin(vstFile, err))
                {
                    int lastIdx = vstRack.getNumPlugins() - 1;
                    vstRack.setEnabled(lastIdx, (bool)item["enabled"]);
                    vstRack.setBypass(lastIdx, (bool)item["bypassed"]);
                }
            }
            else
            {
                missingPlugins.add(item["name"].toString() + " (" + path + ")");
            }
        }
    }

    currentProjectFile = file;
    return true;
}
