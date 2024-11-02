#ifndef CHROMASOUND_DUAL_H
#define CHROMASOUND_DUAL_H

#include "chromasound.h"

class Chromasound_Dual : public Chromasound
{
    public:
    Chromasound_Dual(Chromasound* chromasound1, Chromasound* chromasound2);
        ~Chromasound_Dual();

        // Chromasound interface
    public:
        uint32_t position();
        void setPosition(const float pos);
        void play(const char* vgm, size_t vgmSize, const int currentOffsetSamples, const int currentOffsetData, const bool isSelection);
        void play();
        void pause();
        void stop();
        bool isPlaying() const;
        bool isPaused() const;

        sig_pcm_upload_started signalPcmUploadStarted();
        sig_pcm_upload_finished signalPcmUploadFinished();

    private:
        Chromasound* _chromasound1;
        Chromasound* _chromasound2;
};

#endif // CHROMASOUND_DUAL_H
