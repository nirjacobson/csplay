#include "vgmplayer.h"
#include "spi.h"
#include "chromasound/chromasound_direct.h"

int VGMPlayer::SPI_DELAY = 40;

VGMPlayer::VGMPlayer(Chromasound_Direct& chromasoundDirect)
    : _chromasoundDirect(chromasoundDirect)
    ,_time(0)
    , _position(0)
    , _stop(false)
    , _paused(false)
    , _loopOffsetSamples(0)
    , _loopOffsetData(0)
    , _playing(false)
    , _lastPCMBlockChecksum(0)
    , _fillWithPCM(false)
    , _thread(nullptr)
    , _vgm(nullptr)
    , _pcmBlock(nullptr)
{
    _spiFd = spi_init();
}

VGMPlayer::~VGMPlayer()
{
    spi_close(_spiFd);

    if (_thread) delete _thread;

    if (_vgm) delete _vgm;
    if (_pcmBlock) delete _pcmBlock;
}

void VGMPlayer::setVGM(const char* vgm, const size_t size, const int currentOffsetData)
{
    int _currentOffsetData = currentOffsetData;

    _length = *(uint32_t*)&vgm[0x18];
    _loopLength = *(uint32_t*)&vgm[0x20];
    _introLength = _length - _loopLength;

    uint32_t gd3Offset = *(uint32_t*)&vgm[0x14] + 0x14;
    uint32_t dataOffset = *(uint32_t*)&vgm[0x34] + 0x34;

    _vgmLock.lock();

    _vgmSize = size;

    if (dataOffset == size) {
        _vgm = nullptr;
        _vgmSize = 0;
        _loopOffsetData = -1;
        _loopOffsetSamples = -1;
        _position = 0;
        _vgmLock.unlock();
        return;
    } else if (vgm[dataOffset] == 0x67) {
        uint32_t pcmSize = *(uint32_t*)&vgm[dataOffset + 3];
        _pcmBlockSize = 7 + pcmSize;
        _pcmBlock = new char[_pcmBlockSize];
        memcpy(_pcmBlock, vgm + dataOffset, _pcmBlockSize);

        _vgmSize = gd3Offset - dataOffset - 7 - pcmSize;
        _vgm = new char[_vgmSize];
        memcpy(_vgm, vgm + dataOffset + 7 + pcmSize, _vgmSize);


        if (_currentOffsetData != 0) {
            _currentOffsetData -= dataOffset + 7 + size;
        }
    } else {
        if (_pcmBlock) delete [] _pcmBlock;
        _pcmBlock = nullptr;

        _vgmSize = gd3Offset - dataOffset;
        _vgm = new char[_vgmSize];
        memcpy(_vgm, vgm + dataOffset, gd3Offset - dataOffset);

        if (_currentOffsetData != 0) {
            _currentOffsetData -= dataOffset;
        }
    }

    _loopOffsetData = *(uint32_t*)&vgm[0x1C] + 0x1C - dataOffset - _pcmBlockSize;
    _loopOffsetSamples = _introLength;

    if (_loopOffsetData < 0) {
        _loopOffsetData = -1;
        _loopOffsetSamples = -1;
    }

    _position = _currentOffsetData;

    _vgmLock.unlock();
}

bool VGMPlayer::isPlaying() const
{
    return _playing; // include that thread is running
}

bool VGMPlayer::isPaused() const
{
    return _paused;
}

void VGMPlayer::start() {
    _thread = new std::thread(&VGMPlayer::run, this);
}

void VGMPlayer::stop()
{
    _stopLock.lock();
    _stop = true;
    _paused = false;
    _stopLock.unlock();

    _position = 0;
    _playing = false;
}

void VGMPlayer::pause()
{
    _stopLock.lock();
    _stop = true;
    _paused = true;
    _stopLock.unlock();

    _playing = false;
}

void VGMPlayer::wait()
{
    _thread->join();
}

uint32_t VGMPlayer::length() const
{
    return _length;
}

uint32_t VGMPlayer::introLength() const
{
    return _introLength;
}

uint32_t VGMPlayer::loopLength() const
{
    return _loopLength;
}

uint32_t VGMPlayer::time()
{
    _timeLock.lock();
    uint32_t time = _time;
    _timeLock.unlock();

    if (_playing) {
        struct timeval oldTime = _timer;
        struct timeval now;
        gettimeofday(&now, NULL);
        struct timeval elapsed;
        timersub(&now, &oldTime, &elapsed);
        long microsElapsed = (elapsed.tv_sec * 1000000 + elapsed.tv_usec);
        
        return time + ((float)microsElapsed / 1e6 * 44100);
    } else {
        return time;
    }
}

void VGMPlayer::setTime(const uint32_t time)
{
    _timeLock.lock();
    _time = time;
    _timeLock.unlock();
}

void VGMPlayer::fillWithPCM(const bool enable)
{
    _fillWithPCM = enable;
}

uint32_t VGMPlayer::fletcher32(const char* data, const size_t size)
{
    uint32_t c0, c1, i;
    uint32_t len = size;
    len = (len + 1) & ~1;

    for (c0 = c1 = i = 0; len > 0; ) {
        uint32_t blocklen = len;
        if (blocklen > 360*2) {
            blocklen = 360*2;
        }
        len -= blocklen;

        do {
            c0 = c0 + data[i++];
            c1 = c1 + c0;
        } while ((blocklen -= 2));

        c0 = c0 % 65535;
        c1 = c1 % 65535;
    }

    return ((c1 << 16) | c0);
}

void VGMPlayer::spi_write_wait(uint8_t val)
{
    struct timeval oldTime = _spiTimer;
    struct timeval now;
    gettimeofday(&now, NULL);
    struct timeval elapsed;
    timersub(&now, &oldTime, &elapsed);
    long microsElapsed = (elapsed.tv_sec * 1000000 + elapsed.tv_usec);

    while (microsElapsed < SPI_DELAY) {
        oldTime = _spiTimer;
        gettimeofday(&now, NULL);
        elapsed;
        timersub(&now, &oldTime, &elapsed);
        microsElapsed = (elapsed.tv_sec * 1000000 + elapsed.tv_usec);
    }

    spi_write(_spiFd, val);
    
    _spiTimer = now;
}

void VGMPlayer::spi_xfer_wait(uint8_t* tx, uint8_t* rx)
{
    struct timeval oldTime = _spiTimer;
    struct timeval now;
    gettimeofday(&now, NULL);
    struct timeval elapsed;
    timersub(&now, &oldTime, &elapsed);
    long microsElapsed = (elapsed.tv_sec * 1000000 + elapsed.tv_usec);

    while (microsElapsed < SPI_DELAY) {
        oldTime = _spiTimer;
        gettimeofday(&now, NULL);
        elapsed;
        timersub(&now, &oldTime, &elapsed);
        microsElapsed = (elapsed.tv_sec * 1000000 + elapsed.tv_usec);
    }

    spi_xfer(_spiFd, tx, rx);
    
    _spiTimer = now;
}

void VGMPlayer::run() {
    runPlayback();
}

void VGMPlayer::runPlayback()
{
    uint8_t rx, tx;
    uint16_t space;

    bool loop = _loopOffsetData >= 0 && _loopOffsetSamples >= 0;
    bool paused = _paused;

    _paused = false;
    _stop = false;

    if (paused) {
        spi_write_wait(PAUSE_RESUME);
    } else {
        spi_write_wait(RESET);

        if (_pcmBlock != nullptr) {
            uint32_t lastPCMBlockChecksum = _lastPCMBlockChecksum;

            _lastPCMBlockChecksum = fletcher32(_pcmBlock, _pcmBlockSize);

            if (lastPCMBlockChecksum != _lastPCMBlockChecksum) {
                _chromasoundDirect.notifyStarted();
                uint32_t position = 0;
                while (true) {
                    _stopLock.lock();
                    bool stop = _stop;
                    _stopLock.unlock();
                    if (stop) {
                        return;
                    }

                    spi_write_wait(REPORT_SPACE);
                    spi_xfer_wait(&tx, &rx);
                    space = rx;
                    spi_xfer_wait(&tx, &rx);
                    space |= (int)rx << 8;

                    if (space > 0) {
                        int remaining = _pcmBlockSize - position;
                        long count = space < remaining ? space : remaining;

                        if (count > 0) {
                            spi_write_wait(RECEIVE_DATA);

                            for (int i = 0; i < 4; i++) {
                                spi_write_wait(((char*)&count)[i]);
                            }

                            for (int i = 0; i < count; i++) {
                                spi_write_wait(_pcmBlock[position++]);
                            }
                        }

                        if (count == remaining) {
                            break;
                        }
                    }
                }
                _chromasoundDirect.notifyFinished();
            }
        }
    }

    gettimeofday(&_timer, NULL);
    _playing = true;

    while (true) {
        _stopLock.lock();
        bool stop = _stop;
        bool paused = _paused;
        _stopLock.unlock();
        if (stop) {
            spi_write_wait(paused ? PAUSE_RESUME : STOP);
            if (!paused) {
                _timeLock.lock();
                _time = 0;
                _timeLock.unlock();
                _position = 0;
            }
            return;
        }

        spi_write_wait(REPORT_SPACE);
        spi_xfer_wait(&tx, &rx);
        space = rx;
        spi_xfer_wait(&tx, &rx);
        space |= (int)rx << 8;

        if (space > 0) {
            _vgmLock.lock();
            int remaining = _vgmSize - _position;
            long count = space < remaining ? space : remaining;

            if (count > 0) {
                tx = RECEIVE_DATA;
                spi_write_wait(tx);

                for (int i = 0; i < 4; i++) {
                    tx = ((char*)&count)[i];
                    spi_write_wait(tx);
                }

                for (int i = 0; i < count; i++) {
                    spi_write_wait(_vgm[_position++]);
                }
            }
            _vgmLock.unlock();

            if (loop && count == remaining) {
                _position = _loopOffsetData;
            }
        }

        _timeLock.lock();
        spi_write_wait(REPORT_TIME);
        spi_xfer_wait(&tx, &rx);
        _time = rx;
        spi_xfer_wait(&tx, &rx);
        _time |= (int)rx << 8;
        spi_xfer_wait(&tx, &rx);
        _time |= (int)rx << 16;
        spi_xfer_wait(&tx, &rx);
        _time |= (int)rx << 24;
        gettimeofday(&_timer, NULL);
        _timeLock.unlock();
    }
}
