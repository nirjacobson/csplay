#include "chromasound_direct.h"
#include "direct/gpio.h"

Chromasound_Direct::Chromasound_Direct()
    : _timeOffset(0)
{
    _gpioFd = gpio_init();

    _dispatcherStarted.connect(sigc::mem_fun(*this, &Chromasound_Direct::on_start_notification_from_player));
    _dispatcherFinished.connect(sigc::mem_fun(*this, &Chromasound_Direct::on_finish_notification_from_player));

    _vgmPlayer = new VGMPlayer(*this);
    reset();

    _vgmPlayer->start();
}

Chromasound_Direct::~Chromasound_Direct()
{
    if (_vgmPlayer->isPlaying()) {
        _vgmPlayer->stop();
        _vgmPlayer->wait();
    }

    delete _vgmPlayer;

    gpio_close(_gpioFd);
}

uint32_t Chromasound_Direct::position()
{
    uint32_t time = _vgmPlayer->time();
    uint32_t introLength = _vgmPlayer->introLength();
    uint32_t loopLength = _vgmPlayer->loopLength();
    uint32_t length = _vgmPlayer->length();

    if (_vgmPlayer->loopLength() <= 0) {
        if (_vgmPlayer->isPlaying() && (time + _timeOffset) >= _vgmPlayer->length()) {
            stop();
            return 0;
        }
        return (time + _timeOffset);
    }

    if (_vgmPlayer->introLength() <= 0) {
        return (_timeOffset + (time % length));
    } else {
        uint32_t t = _timeOffset + time;
        return ((t < introLength)
                ? t
                : (((t - introLength) % loopLength) + introLength));
    }
}

void Chromasound_Direct::setPosition(const float pos)
{

}

void Chromasound_Direct::play(const char* vgm, size_t vgmSize, const int currentOffsetSamples, const int currentOffsetData, const bool isSelection)
{
    if (isSelection) {
        _timeOffset = currentOffsetSamples;
    }

    if (_vgmPlayer->isPlaying()) {
        _vgmPlayer->stop();
        _vgmPlayer->wait();
    }

    reset();

    _vgmPlayer->setVGM(vgm, vgmSize, currentOffsetData);
    _vgmPlayer->start();
}

void Chromasound_Direct::play()
{
    _vgmPlayer->start();
}

void Chromasound_Direct::pause()
{
    _vgmPlayer->pause();
}

void Chromasound_Direct::stop()
{
    _vgmPlayer->stop();
    _vgmPlayer->wait();

    reset();

    _timeOffset = 0;
}

bool Chromasound_Direct::isPlaying() const
{
    return _vgmPlayer->isPlaying();
}

bool Chromasound_Direct::isPaused() const
{
    return _vgmPlayer->isPaused();
}


Chromasound::sig_pcm_upload_started Chromasound_Direct::signalPcmUploadStarted() {
    return _signalPcmUploadStarted;
}

Chromasound::sig_pcm_upload_finished Chromasound_Direct::signalPcmUploadFinished() {
    return _signalPcmUploadFinished;
}

void Chromasound_Direct::reset()
{
    gpio_write(_gpioFd, 2, 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    gpio_write(_gpioFd, 2, 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

void Chromasound_Direct::notifyStarted() {
    _dispatcherStarted.emit();
}

void Chromasound_Direct::notifyFinished() {
    _dispatcherFinished.emit();
}

void Chromasound_Direct::on_start_notification_from_player() {
    _signalPcmUploadStarted.emit();
}

void Chromasound_Direct::on_finish_notification_from_player() {
    _signalPcmUploadFinished.emit();
}