#include "MusicPlayer.h"

MusicPlayer::MusicPlayer()
{
    formatManager.registerBasicFormats();
}

MusicPlayer::~MusicPlayer()
{
    transportSource.setSource(nullptr);
}

void MusicPlayer::prepare(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    transportSource.prepareToPlay(samplesPerBlock, sampleRate);
}

void MusicPlayer::release()
{
    transportSource.releaseResources();
}

void MusicPlayer::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    if (transportSource.isPlaying())
    {
        transportSource.getNextAudioBlock(bufferToFill);

        // Apply music bus volume gain
        if (bufferToFill.buffer != nullptr)
        {
            bufferToFill.buffer->applyGain(bufferToFill.startSample, bufferToFill.numSamples, volumeGain);
        }
    }
    else
    {
        bufferToFill.clearActiveBufferRegion();
    }
}

bool MusicPlayer::loadFile(const juce::File& file)
{
    if (!file.existsAsFile())
        return false;

    auto* reader = formatManager.createReaderFor(file);
    if (reader != nullptr)
    {
        transportSource.stop();
        transportSource.setSource(nullptr);
        readerSource = std::make_unique<juce::AudioFormatReaderSource>(reader, true);
        transportSource.setSource(readerSource.get(), 0, nullptr, reader->sampleRate);
        currentFile = file;

        // Add to playlist if not already there
        bool inList = false;
        for (size_t i = 0; i < playlist.size(); ++i)
        {
            if (playlist[i] == file)
            {
                currentPlaylistIndex = (int)i;
                inList = true;
                break;
            }
        }
        if (!inList)
        {
            playlist.push_back(file);
            currentPlaylistIndex = (int)playlist.size() - 1;
        }

        sendChangeMessage();
        return true;
    }

    return false;
}

void MusicPlayer::play()
{
    transportSource.start();
    sendChangeMessage();
}

void MusicPlayer::pause()
{
    transportSource.stop();
    sendChangeMessage();
}

void MusicPlayer::stop()
{
    transportSource.stop();
    transportSource.setPosition(0.0);
    sendChangeMessage();
}

bool MusicPlayer::isPlaying() const
{
    return transportSource.isPlaying();
}

void MusicPlayer::setPosition(double posSeconds)
{
    transportSource.setPosition(posSeconds);
}

double MusicPlayer::getCurrentPosition() const
{
    return transportSource.getCurrentPosition();
}

double MusicPlayer::getDuration() const
{
    return transportSource.getLengthInSeconds();
}

void MusicPlayer::addTrackToPlaylist(const juce::File& file)
{
    if (file.existsAsFile())
    {
        playlist.push_back(file);
        if (playlist.size() == 1)
        {
            loadFile(file);
        }
    }
}

void MusicPlayer::playNext()
{
    if (playlist.empty()) return;
    currentPlaylistIndex = (currentPlaylistIndex + 1) % (int)playlist.size();
    if (loadFile(playlist[currentPlaylistIndex]))
        play();
}

void MusicPlayer::playPrevious()
{
    if (playlist.empty()) return;
    currentPlaylistIndex = (currentPlaylistIndex - 1 + (int)playlist.size()) % (int)playlist.size();
    if (loadFile(playlist[currentPlaylistIndex]))
        play();
}
