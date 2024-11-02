#ifndef CHROMASOUND_DIRECT_H
#define CHROMASOUND_DIRECT_H

#include <mutex>
#include <glibmm.h>

#include "chromasound.h"
#include "direct/vgmplayer.h"

class Chromasound_Direct : public Chromasound
{
    public:
        Chromasound_Direct();
        ~Chromasound_Direct();

        uint32_t position();
        void setPosition(const float pos);
        void play(const char* vgm, size_t vgmSize, const int currentOffsetSamples, const int currentOffsetData, const bool isSelection = false);
        void play();
        void pause();
        void stop();
        bool isPlaying() const;
        bool isPaused() const;

        void notifyStarted();
        void notifyFinished();

        sig_pcm_upload_started signalPcmUploadStarted();
        sig_pcm_upload_finished signalPcmUploadFinished();

    private:
        int _gpioFd;

        std::mutex _mutex;

        VGMPlayer* _vgmPlayer;

        long _timeOffset;
        
        sig_pcm_upload_started _signalPcmUploadStarted;
        sig_pcm_upload_finished _signalPcmUploadFinished;

        Glib::Dispatcher _dispatcherStarted;
        Glib::Dispatcher _dispatcherFinished;

        void on_start_notification_from_player();
        void on_finish_notification_from_player();

        void reset();
};

#endif // CHROMASOUND_DIRECT_H
