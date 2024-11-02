#include "chromasound_dual.h"
#include "chromasound_emu.h"

Chromasound_Dual::Chromasound_Dual(Chromasound* chromasound1, Chromasound* chromasound2)
    : _chromasound1(chromasound1)
    , _chromasound2(chromasound2)
{

}

Chromasound_Dual::~Chromasound_Dual()
{
    delete _chromasound1;
    delete _chromasound2;
}

uint32_t Chromasound_Dual::position()
{
    return _chromasound1->position();
}

void Chromasound_Dual::setPosition(const float pos)
{
    _chromasound1->setPosition(pos);
}

void Chromasound_Dual::play(const char* vgm, size_t vgmSize, const int currentOffsetSamples, const int currentOffsetData, const bool isSelection)
{
    _chromasound1->play(vgm, vgmSize, currentOffsetSamples, currentOffsetData, isSelection);
}

void Chromasound_Dual::play()
{
    _chromasound1->play();
}

void Chromasound_Dual::pause()
{
    _chromasound1->pause();
}

void Chromasound_Dual::stop()
{
    _chromasound1->stop();
    _chromasound2->stop();
}

bool Chromasound_Dual::isPlaying() const
{
    return _chromasound1->isPlaying();
}

bool Chromasound_Dual::isPaused() const
{
    return _chromasound1->isPaused();
}

Chromasound::sig_pcm_upload_started Chromasound_Dual::signalPcmUploadStarted() {
    return _chromasound1->signalPcmUploadStarted();
}

Chromasound::sig_pcm_upload_finished Chromasound_Dual::signalPcmUploadFinished() {
    return _chromasound1->signalPcmUploadFinished();
}