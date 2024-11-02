#include "chromasound_standin.h"

uint32_t Chromasound_Standin::position()
{
    float sampleUS = 1e6f / 44100;

    struct timeval oldTime = _timer;
    struct timeval now;
    gettimeofday(&now, NULL);
    struct timeval elapsed;
    timersub(&now, &oldTime, &elapsed);
    long microsElapsed = (elapsed.tv_sec * 1000000 + elapsed.tv_usec);

    uint32_t pos = (_ref + microsElapsed) / sampleUS;

    if (!_playing) {
        pos = _ref / sampleUS;
    }

    return pos;
}

void Chromasound_Standin::setPosition(const float pos)
{
    _ref = pos;
}

bool Chromasound_Standin::isPlaying() const
{
    return _playing;
}

bool Chromasound_Standin::isPaused() const
{
    return _paused;
}

Chromasound_Standin::Chromasound_Standin()
    : _ref(0)
    , _playing(false)
    , _loopOffsetSamples(-1)
    , _paused(false)
{

}

void Chromasound_Standin::play(const char* vgm, size_t, const int, const int, const bool)
{
    Glib::Dispatcher dispatcher;
    dispatcher.connect([this] {
        _signalPcmUploadFinished.emit();
    });

    auto finishLater = std::thread([&](){
        std::this_thread::sleep_for(std::chrono::seconds(3));
        dispatcher.emit();
    });
    _signalPcmUploadStarted.emit();

    uint32_t length = *(uint32_t*)&vgm[0x18];
    uint32_t loopLength = *(uint32_t*)&vgm[0x20];
    uint32_t introLength = length - loopLength;

    _loopOffsetSamples = introLength;
    _playing = true;
    _paused = false;
    gettimeofday(&_timer, NULL);

    finishLater.join();
}

void Chromasound_Standin::play()
{
    Glib::Dispatcher dispatcher;
    dispatcher.connect([this] {
        _signalPcmUploadFinished.emit();
    });

    auto finishLater = std::thread([&](){
        std::this_thread::sleep_for(std::chrono::seconds(3));
        dispatcher.emit();
    });
    _signalPcmUploadStarted.emit();

    _playing = true;
    _paused = false;
    gettimeofday(&_timer, NULL);

    finishLater.join();
}

void Chromasound_Standin::pause()
{
    struct timeval oldTime = _timer;
    struct timeval now;
    gettimeofday(&now, NULL);
    struct timeval elapsed;
    timersub(&now, &oldTime, &elapsed);
    long microsElapsed = (elapsed.tv_sec * 1000000 + elapsed.tv_usec);

    _ref += microsElapsed;
    _playing = false;
    _paused = true;
}

void Chromasound_Standin::stop()
{
    _playing = false;
    _paused = false;
    _ref = 0;
}

Chromasound::sig_pcm_upload_started Chromasound_Standin::signalPcmUploadStarted() {
    return _signalPcmUploadStarted;
}

Chromasound::sig_pcm_upload_finished Chromasound_Standin::signalPcmUploadFinished() {
    return _signalPcmUploadFinished;
}