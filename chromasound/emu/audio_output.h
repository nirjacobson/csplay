#ifndef AUDIO_OUTPUT_H
#define AUDIO_OUTPUT_H

#include <iostream>
#include <portaudio.h>
#include <stdint.h>
#include <string.h>
#include <vector>
#include <algorithm>
#include <sstream>

#include "consumer.h"

template <typename T>
class AudioOutput : public Consumer<T> {
    public:
        AudioOutput();

        static AudioOutput* instance();

        void init();
        void open(const std::string& device, const int framesPerBuffer);
        void destroy();

        std::vector<std::string> devices();
        int defaultDeviceIndex();
        std::string defaultDevice();

        void start();
        void stop();
        void restart();

        bool isInited();
        bool isDefault();
        bool isRunning();

        static constexpr int SAMPLE_RATE = 44100;

    private:
        static AudioOutput* _instance;

        PaError _paError;
        PaStream* _paStream;

        bool _inited;

        bool _running;

        int _framesPerBuffer;

        static int paCallback(const void* inputBuffer,
                              void* outputBuffer,
                              unsigned long framesPerBuffer,
                              const PaStreamCallbackTimeInfo* timeInfo,
                              PaStreamCallbackFlags statusFlags,
                              void* userData);
};

template <typename T>
AudioOutput<T>::AudioOutput()
    : _inited(false)
    , _running(false)
{

}

template <typename T>
bool AudioOutput<T>::isInited()
{
    return _inited;
}

template <typename T>
AudioOutput<T>* AudioOutput<T>::_instance = nullptr;

template <typename T>
AudioOutput<T>* AudioOutput<T>::instance() {
    if (AudioOutput::_instance == nullptr) {
        AudioOutput::_instance = new AudioOutput;
    }

    return _instance;
}

template <typename T>
void AudioOutput<T>::init() {
    _paError = Pa_Initialize();
    if( _paError != paNoError ) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText( _paError ) << std::endl;
        return;
    }
    _inited = true;
}

template <typename T>
void AudioOutput<T>::open(const std::string& device, const int framesPerBuffer) {
    _framesPerBuffer = framesPerBuffer;

    _running = false;

    PaSampleFormat format;
    if (std::is_same<T, int16_t>::value)
        format = paInt16;
    else if (std::is_same<T, float>::value)
        format = paFloat32;

    /* Open an audio I/O stream. */
    PaStreamParameters outputParams;

    memset(&outputParams, 0, sizeof(outputParams));

    std::vector<std::string> devs = devices();
    outputParams.channelCount = 2;
    outputParams.device = std::find(devs.begin(), devs.end(), device) - devs.begin();
    outputParams.sampleFormat = format;

    _paError = Pa_OpenStream(&_paStream,
                             NULL,
                             &outputParams,
                             SAMPLE_RATE,
                             framesPerBuffer,
                             paNoFlag,
                             &paCallback,
                             nullptr);

    if( _paError != paNoError ) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText( _paError ) << std::endl;
        return;
    }
}

template <typename T>
std::vector<std::string> AudioOutput<T>::devices() {
    std::vector<std::string> devices;

    int count = Pa_GetDeviceCount();
    for (int i = 0; i < count; i++) {
        const PaDeviceInfo* info = Pa_GetDeviceInfo(i);
        devices.push_back(info->name);
    }

    for (int i = 0; i < count; i++) {
        int devCount = 1;
        for (int j = i + 1; j < count; j++) {
            if (devices[i].starts_with(devices[j])) {
                if (devCount == 1) {
                    std::stringstream ss;
                    ss << devices[i] << " #" << devCount++;
                    devices[i] = ss.str();
                }

                std::stringstream ss;
                ss << devices[j] << " #" << devCount++;
                devices[j] = ss.str();
            }
        }
    }

    return devices;
}

template <typename T>
int AudioOutput<T>::defaultDeviceIndex() {
    return Pa_GetDefaultOutputDevice();
}

template <typename T>
std::string AudioOutput<T>::defaultDevice() {
    return devices()[defaultDeviceIndex()];
}

template <typename T>
void AudioOutput<T>::destroy() {
    _paError = Pa_CloseStream( _paStream );
    if( _paError != paNoError ) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText( _paError ) << std::endl;
        return;
    }

    _paError = Pa_Terminate();
    if( _paError != paNoError ) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText( _paError ) << std::endl;
        return;
    }

    delete _instance;
    _instance = nullptr;
    _inited = false;
}

template <typename T>
void AudioOutput<T>::start() {
    _paError = Pa_StartStream( _paStream );
    if( _paError != paNoError ) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText( _paError ) << std::endl;
        return;
    } else {
        _running = true;
    }
}

template <typename T>
void AudioOutput<T>::stop() {
    _paError = Pa_StopStream( _paStream );
    if( _paError != paNoError ) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText( _paError ) << std::endl;
        return;
    } else {
        _running = false;
    }
}

template <typename T>
void AudioOutput<T>::restart() {
    bool started = _running;

    if (started) {
        stop();
    }

    _paError = Pa_CloseStream( _paStream );
    if( _paError != paNoError ) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText( _paError ) << std::endl;
        return;
    }

    _paError = Pa_Terminate();
    if( _paError != paNoError ) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText( _paError ) << std::endl;
        return;
    }

    init(_framesPerBuffer);

    if (started) {
        start();
    }
}

template <typename T>
bool AudioOutput<T>::isDefault() {
    return Pa_GetDefaultOutputDevice() == 0;
}

template<typename T>
bool AudioOutput<T>::isRunning()
{
    return _running;
}

template <typename T>
int AudioOutput<T>::paCallback(const void* inputBuffer,
                               void* outputBuffer,
                               unsigned long framesPerBuffer,
                               const PaStreamCallbackTimeInfo* timeInfo,
                               PaStreamCallbackFlags statusFlags,
                               void* userData) {
    T* out = (T*)outputBuffer;
    unsigned int i, j;
    (void) inputBuffer; /* Prevent unused variable warning. */
    (void) userData;

    bool activeA = instance()->producerA() && instance()->producerA()->active();
    bool activeB = instance()->producerB() && instance()->producerB()->active();

    T* bufferA = activeA ? instance()->consumeA(framesPerBuffer * 2) : nullptr;
    T* bufferB = activeB ? instance()->consumeB(framesPerBuffer * 2) : nullptr;

    for( i=0, j=0; i<framesPerBuffer; i++)
    {
        *out++ = (activeA ? bufferA[j] : 0) + (activeB ? bufferB[j] : 0); // left
        j++;

        *out++ = (activeA ? bufferA[j] : 0) + (activeB ? bufferB[j] : 0); // right
        j++;
    }

    return 0;
}

#endif // AUDIO_OUTPUT_H
