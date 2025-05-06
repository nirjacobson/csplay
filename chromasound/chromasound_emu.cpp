#include "chromasound_emu.h"
#include "emu/Vgm_Emu.h"
#include "emu/Data_Reader.h"

static bool log_err(blargg_err_t err)
{
    if (err)
        fprintf(stderr, "%s\n", err);
    return !!err;
}

static void log_warning(Music_Emu * emu)
{
    const char *str = emu->warning();
    if (str)
        fprintf(stderr, "%s\n", str);
}

Chromasound_Emu::Chromasound_Emu()
    : _position(0)
    , _positionOffset(0)
    , _buffer(0)
    , _bufferIdx(0)
    , _player(new Player(*this))
    , _stopped(true)
    , _paused(false)
    , _haveInfo(false)
    , _isSelection(false)
{
    _type = gme_vgm_type;
    _emu = gme_new_emu(_type, 44100);
    _emu->ignore_silence(true);

    _buffers[0] = nullptr;
    _buffers[1] = nullptr;
    setBufferSizes();
    setEqualizer();

}

Chromasound_Emu::~Chromasound_Emu()
{
    _player->action(Player::Action::Exit);
    _player->wait();

    delete _player;

    delete _buffers[0];
    delete _buffers[1];

    gme_delete(_emu);
}

Chromasound::sig_pcm_upload_started Chromasound_Emu::signalPcmUploadStarted() {
    return _signalPcmUploadStarted;
}

Chromasound::sig_pcm_upload_finished Chromasound_Emu::signalPcmUploadFinished() {
    return _signalPcmUploadFinished;
}

void Chromasound_Emu::setEqualizer()
{
    int _bass = 0;
    int _treble = 0;

    Music_Emu::equalizer_t eq;

    // bass - logarithmic, 2 to 8194 Hz
    double bass = 1.0 - (_bass / 200.0 + 0.5);
    eq.bass = (long) (2.0 + pow( 2.0, bass * 13 ));

    // treble - -50 to 0 to +5 dB
    double treble = _treble / 100.0;
    eq.treble = treble * (treble < 0 ? 50.0 : 5.0);

    _emu->set_equalizer(eq);
}

void Chromasound_Emu::setBufferSizes()
{
    int audioBufferSize = 1024;
    int readBufferSize = 1;

    _framesPerReadBuffer = audioBufferSize * readBufferSize;

    if (_buffers[0] != nullptr && _buffers[1] != nullptr) {
        delete _buffers[0];
        delete _buffers[1];
    }

    _buffers[0] = new Music_Emu::sample_t[_framesPerReadBuffer * 2];
    _buffers[1] = new Music_Emu::sample_t[_framesPerReadBuffer * 2];
    memset(_buffers[0], 0, _framesPerReadBuffer * 2 * sizeof(int16_t));
    memset(_buffers[1], 0, _framesPerReadBuffer * 2 * sizeof(int16_t));
}

uint32_t Chromasound_Emu::position()
{
    uint32_t pos = _positionOffset + _position;

    if (!_haveInfo) {
        return pos;
    }

    if (_info.loop_length <= 0) {
        uint32_t lengthSamples = _info.length / 1000.0f * 44100;
        if (pos >= lengthSamples) {
            stop();
            return 0;
        }
        return pos;
    }

    uint32_t loopLengthSamples = _info.loop_length / 1000.0f * 44100;
    if (_info.intro_length <= 0) {
        if (_isSelection) {
            return _positionOffset + (_position % loopLengthSamples);
        }
        return pos % loopLengthSamples;
    } else {
        uint32_t introLengthSamples = _info.intro_length / 1000.0f * 44100;
        return ((pos < introLengthSamples)
                ? pos
                : (((pos - introLengthSamples) % loopLengthSamples) + introLengthSamples));
    }
}

void Chromasound_Emu::setPosition(const float pos)
{

}

void Chromasound_Emu::play(const char* vgm, size_t vgmSize, const int currentOffsetSamples, const int currentOffsetData, const bool isSelection)
{
    Vgm_Emu_Impl* impl = dynamic_cast<Vgm_Emu_Impl*>(_emu);

    deactivate();
    Mem_File_Reader reader(vgm, vgmSize);
    if (log_err(_emu->load(reader)))
        return;

    log_warning(_emu);

    // start track
    if (log_err(_emu->start_track(0)))
        return;

    log_warning(_emu);

    _emu->track_info(&_info);
    _haveInfo = true;

    _isSelection = isSelection;

    _position = 0;
    if (isSelection) {
        _positionOffset = currentOffsetSamples;
    } else {
        if (log_err(_emu->skip(currentOffsetSamples * 2)))
            return;
    }

    setEqualizer();

    _loadLock.lock();
    _bufferIdx = 0;
    _player->buffer(_buffer);
    _player->action(Player::Action::Load);
    _loadLock.unlock();

    _stopped = false;
    _paused = false;
    activate();
}

void Chromasound_Emu::play()
{
    deactivate();
    setEqualizer();

    _stopped = false;
    _paused = false;
    activate();
}

void Chromasound_Emu::pause()
{
    _stopped = true;
    _paused = true;
    deactivate();
}

void Chromasound_Emu::stop()
{
    _stopped = true;
    _paused = false;
    _position = 0;
    _positionOffset = 0;
    char data[129];
    VGM::generateHeader(data, 1, -1, 0, 0, false);
    data[128] = 0x66;

    Mem_File_Reader reader(data, sizeof(data));

    if (log_err(_emu->load(reader)))
        return;

    log_warning(_emu);

    Vgm_Emu_Impl* impl = dynamic_cast<Vgm_Emu_Impl*>(_emu);
    impl->reset();

    // start track
    if (log_err(_emu->start_track(0)))
        return;

    log_warning(_emu);

    setEqualizer();

    signalStopped().emit();
}

bool Chromasound_Emu::isPlaying() const
{
    return !_stopped;
}

bool Chromasound_Emu::isPaused() const
{
    return _paused;
}

int16_t* Chromasound_Emu::next(int size)
{
    if (_bufferIdx == 0) {
        _loadLock.lock();
        _player->buffer(1 - _buffer);
        _player->action(Player::Action::Load);
        _loadLock.unlock();
    }

    int16_t* addr = &_buffers[_buffer][_bufferIdx];

    _bufferIdx += size;
    _bufferIdx %= _framesPerReadBuffer * 2;

    if (!_stopped) {
        _position += size / 2;
    }

    if (_bufferIdx == 0) {
        _buffer = 1 - _buffer;
    }

    return addr;
}

Chromasound_Emu::Player::Player(Chromasound_Emu& emu)
    : _emu(emu)
    , _action(Action::Idle)
    , _buffer(emu._buffer)
    , _thread(&Chromasound_Emu::Player::run, this)
{

}

void Chromasound_Emu::Player::action(const Action action)
{
    _actionLock.lock();
    _action = action;
    _actionLock.unlock();
}

Chromasound_Emu::Player::Action Chromasound_Emu::Player::action()
{
    _actionLock.lock();
    Action a = _action;
    _actionLock.unlock();

    return a;
}

void Chromasound_Emu::Player::buffer(const int buffer)
{
    _buffer = buffer;
}

void Chromasound_Emu::Player::run()
{
    while (true) {
        Action a = action();

        switch (a) {
            case Idle:
                break;
            case Load:
                _emu._loadLock.lock();
                _emu._emu->play(_emu._framesPerReadBuffer * 2, _emu._buffers[_buffer]);
                _emu._loadLock.unlock();

                if (action() == Action::Exit) {
                    return;
                }

                action(Action::Idle);
                break;
            case Exit:
                return;
        }
    }
}

void Chromasound_Emu::Player::wait() {
    _thread.join();
}

