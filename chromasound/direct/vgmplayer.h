#ifndef VGMPLAYER_H
#define VGMPLAYER_H

#include <mutex>
#include <stddef.h>
#include <sigc++/signal.h>
#include <thread>

#ifndef _WIN32
#include <sys/time.h>
#endif

class Chromasound_Direct;

class VGMPlayer
{
    public:
        explicit VGMPlayer(Chromasound_Direct& chromasoundDirect);
        ~VGMPlayer();

        void setVGM(const char* vgm, const size_t size, const int currentOffsetData);

        bool isPlaying() const;
        bool isPaused() const;

        void start();

        void stop();
        void pause();

        void wait();

        uint32_t length() const;
        uint32_t introLength() const;
        uint32_t loopLength() const;

        uint32_t time();
        void setTime(const uint32_t time);

        void fillWithPCM(const bool enable);

    private:
        static int SPI_DELAY;

        enum {
            IDLE,
            REPORT_SPACE,
            RECEIVE_DATA,
            REPORT_TIME,
            PAUSE_RESUME,
            STOP,
            RESET,
            FILL_WITH_PCM,
            STOP_FILL_WITH_PCM,
            DISCRETE_PCM,
            INTEGR8D_PCM
        } Command;

        Chromasound_Direct& _chromasoundDirect;

        int _spiFd;

        char* _vgm;
        size_t _vgmSize;
        char* _pcmBlock;
        size_t _pcmBlockSize;
        volatile uint32_t _time;
        uint32_t _position;

        uint32_t _length;
        uint32_t _introLength;
        uint32_t _loopLength;

        std::mutex _stopLock;
        std::mutex _timeLock;
        std::mutex _vgmLock;
        volatile bool _stop;
        volatile bool _paused;

        int _loopOffsetSamples;
        int _loopOffsetData;

        timeval _timer;
        bool _playing;

        timeval _spiTimer;

        bool _fillWithPCM;

        std::thread* _thread;

        uint32_t fletcher32(const char* data, const size_t size);
        uint32_t _lastPCMBlockChecksum;

        void spi_write_wait(uint8_t val);
        void spi_xfer_wait(uint8_t* tx, uint8_t* rx);

        void runPlayback();

        // QThread interface
    protected:
        void run();
};

#endif // VGMPLAYER_H
