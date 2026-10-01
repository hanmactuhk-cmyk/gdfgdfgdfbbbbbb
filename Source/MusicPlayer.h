#pragma once
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <vector>

class MusicPlayer : public juce::ChangeBroadcaster
{
public:
    MusicPlayer();
    ~MusicPlayer() override;

    void prepare(double sampleRate, int samplesPerBlock);
    void release();
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill);

    bool loadFile(const juce::File& file);
    void play();
    void pause();
    void stop();
    bool isPlaying() const;

    void setPosition(double posSeconds);
    double getCurrentPosition() const;
    double getDuration() const;

    void setVolume(float gainLinear) { volumeGain = juce::jmax(0.0f, gainLinear); }
    float getVolume() const noexcept { return volumeGain; }

    juce::String getCurrentTrackName() const { return currentFile.getFileNameWithoutExtension(); }

    void addTrackToPlaylist(const juce::File& file);
    void playNext();
    void playPrevious();
    int getNumTracks() const { return (int)playlist.size(); }
    int getCurrentTrackIndex() const { return currentPlaylistIndex; }

private:
    juce::AudioFormatManager formatManager;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    juce::AudioTransportSource transportSource;

    juce::File currentFile;
    std::vector<juce::File> playlist;
    int currentPlaylistIndex = -1;

    float volumeGain = 0.85f;
    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MusicPlayer)
};
