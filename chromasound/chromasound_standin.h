#ifndef CHROMASOUND_STANDIN_H
#define CHROMASOUND_STANDIN_H

#include <thread>
#include <sigc++/signal.h>
#include <glibmm.h>

#ifndef _WIN32
#include <sys/time.h>
#endif

#include "chromasound.h"

class Chromasound_Standin : public Chromasound
{
    public:
        Chromasound_Standin();

        void play(const char* vgm, size_t, const int, const int, const bool = false);
        void play();
        void pause();
        void stop();
        uint32_t position();
        void setPosition(const float pos);
        bool isPlaying() const;
        bool isPaused() const;

        sig_pcm_upload_started signalPcmUploadStarted();
        sig_pcm_upload_finished signalPcmUploadFinished();

    private:
        timeval _timer;
        uint64_t _ref;
        bool _playing;
        int _loopOffsetSamples;
        bool _paused;

        sig_pcm_upload_started _signalPcmUploadStarted;
        sig_pcm_upload_finished _signalPcmUploadFinished;
};

#endif // CHROMASOUND_STANDIN_H
