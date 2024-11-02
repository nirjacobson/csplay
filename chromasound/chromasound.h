#ifndef CHROMASOUND_H
#define CHROMASOUND_H

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <sigc++/signal.h>

class Chromasound
{
    public:
        typedef sigc::signal<void()> sig_pcm_upload_started;
        typedef sigc::signal<void()> sig_pcm_upload_finished;
        typedef sigc::signal<void()> sig_stopped;

        virtual ~Chromasound() {};

        virtual uint32_t position() = 0;
        virtual void setPosition(const float pos) = 0;

        virtual void play(const char* vgm, size_t vgmSize, const int currentOffsetSamples, const int currentOffsetData, const bool isSelection = false) = 0;
        virtual void play() = 0;
        virtual void pause() = 0;
        virtual void stop() = 0;

        virtual bool isPlaying() const = 0;
        virtual bool isPaused() const = 0;

        virtual sig_pcm_upload_started signalPcmUploadStarted() = 0;
        virtual sig_pcm_upload_finished signalPcmUploadFinished() = 0;
        sig_stopped signalStopped() { return _signalStopped; }

    private:
        sig_stopped _signalStopped;
};

#endif // CHROMASOUND_H
