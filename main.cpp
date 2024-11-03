#include <algorithm>
#include <filesystem>
#include <iostream>
#include <limits>
#include <cstdlib>
#include <fcntl.h>
#include <termios.h>
#include <cstdio>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "chromasound/emu/audio_output.h"
#include "chromasound/chromasound.h"
#include "chromasound/chromasound_emu.h"
#include "chromasound/chromasound_direct.h"
#include "formats/gd3.h"

#define READ_END    0
#define WRITE_END   1

char* getCmdOption(char** begin, char** end, const std::string& option) {
    char** itr = std::find(begin, end, option);
    if (itr != end && ++itr != end) {
        return *itr;
    }
    return 0;
}

bool cmdOptionExists(char** begin, char** end, const std::string& option) {
    return std::find(begin, end, option) != end;
}

bool isRaspberryPi() {
    id_t pid;
    int p[2];

    pipe(p);
    pid = fork();

    if (pid == 0) {
        dup2(p[WRITE_END], STDOUT_FILENO);
        close(p[READ_END]);
        close(p[WRITE_END]);

        char* argv[4] = {
            new char[strlen("grep")+1],
            new char[strlen("Model")+1],
            new char[strlen("/proc/cpuinfo")],
            NULL
        };
        strcpy(argv[0], "grep");
        strcpy(argv[1], "Model");
        strcpy(argv[2], "/proc/cpuinfo");
        execvp("grep", argv);
    } else {
        close(p[WRITE_END]);

        std::stringstream ss;
        char buf[512];
        int bytes;
        while ((bytes = read(p[READ_END], buf, 512)) > 0) {
            buf[bytes] = '\0';
            ss << buf;
        }

        int status;
        close(p[READ_END]);
        waitpid(pid, &status, 0);

        std::string str;
        std::getline(ss, str);

        return str.find("Raspberry Pi") != std::string::npos;
    }
}

void usage() {
    std::cout << "Usage: csplay [options] [files]" << std::endl << std::endl;

    std::cout << "Options:" << std::endl;

    std::cout << std::left << std::setw(20) <<  "\t-emu";
    std::cout << "Play using the emulator on the system's" << std::endl;
    std::cout << std::left << std::setw(20) << '\t';
    std::cout << "default audio output (default)" << std::endl;
    
    if (isRaspberryPi()) {
        std::cout << std::endl;
        std::cout << std::left << std::setw(20) <<  "\t-direct";
        std::cout << "Play using a Chromasound Direct HAT";
        std::cout << std::endl << std::endl;
    }
}

char* pcmToVgm(const char* path, size_t* vgmSize) {
    FILE* pcmFile = fopen(path, "rb");

    fseek(pcmFile, 0, SEEK_END);
    size_t pcmSize = ftell(pcmFile);
    fseek(pcmFile, 0, SEEK_SET);

    char* pcmData = new char[pcmSize];

    size_t bytesRead = 0;
    while (bytesRead < pcmSize) {
        bytesRead += fread(pcmData + bytesRead, 1, pcmSize - bytesRead, pcmFile);
    }

    fclose(pcmFile);

    *vgmSize = 128 + (pcmSize / 2) + 7;

    char* vgm = new char[*vgmSize];

    VGM::generateHeader(vgm, *vgmSize - 128, pcmSize, 0, 0, false);

    vgm[128] = 0x97;
    vgm[129] = 0xFE;
    *(uint32_t*)&vgm[130] = pcmSize / 2;

    for (size_t i = 0, j = 0; i < pcmSize; i += 2, j++) {
        vgm[134 + j] = pcmData[i];
    }

    vgm[*vgmSize - 1] = 0x66;

    free(pcmData);

    return vgm;
}

int main(int argc, char *argv[])
{
    if (argc < 2 || access(argv[argc - 1], F_OK) == -1) {
        usage();
        return 0;
    }

    AudioOutput<int16_t>* audioOutput = nullptr;
    Chromasound* chromasound;

    fcntl(0, F_SETFL, O_NONBLOCK);

    struct termios t;
    struct termios t2;
    tcgetattr(STDIN_FILENO, &t);
    t2 = t;
    t.c_lflag &= ~ICANON;
    t.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &t);

    int i = 1;

    if (cmdOptionExists(argv, argv + argc, "-direct")) {
        i++;
    }
    if (cmdOptionExists(argv, argv + argc, "-emu")) {
        i++;
    }

    if (isRaspberryPi() && cmdOptionExists(argv, argv + argc, "-direct")) {
        chromasound = new Chromasound_Direct;
    } else {
        chromasound = new Chromasound_Emu;
        audioOutput = AudioOutput<int16_t>::instance();

        int stderr_old = dup(STDERR_FILENO);

        freopen("/dev/null", "w", stderr);

        audioOutput->init();
        audioOutput->producer(dynamic_cast<Chromasound_Emu*>(chromasound));

        audioOutput->openDefault(1024);
        audioOutput->start();

        stderr = fdopen(stderr_old, "r");
    }

    for (i; i < argc; i++) {
        std::filesystem::path p = argv[i];
        std::string absolutePath = std::filesystem::absolute(p);
        std::string realPath = realpath(absolutePath.c_str(), nullptr);

        FILE* vgmFile = nullptr;
        char* vgm = nullptr;
        size_t vgmSize;

        if (realPath.ends_with("pcm")) {
            vgm = pcmToVgm(realPath.c_str(), &vgmSize);
        }

        if (!vgm) {
            GD3 gd3 = GD3::parseGd3(realPath);

            std::cout << gd3.title() << std::endl;
            std::cout << gd3.game() << std::endl;
            std::cout << gd3.author() << std::endl;
            std::cout << gd3.releaseDate() << std::endl << std::endl;

            vgmFile = fopen(realPath.c_str(), "rb");

            fseek(vgmFile, 0, SEEK_END);
            vgmSize = ftell(vgmFile);
            fseek(vgmFile, 0, SEEK_SET);

            vgm = new char[vgmSize];

            size_t bytesRead = 0;
            while (bytesRead < vgmSize) {
                bytesRead += fread(vgm + bytesRead, 1, vgmSize - bytesRead, vgmFile);
            }
        } else {
            std::cout << realPath.substr(realPath.find_last_of('/') + 1) << std::endl << std::endl;
        }

        chromasound->play(vgm, vgmSize, 0, 0);

        timeval timer = { 0, 0 };
        bool quit = false;
        while (true) {
            struct timeval oldTime = timer;
            struct timeval now;
            gettimeofday(&now, NULL);
            struct timeval elapsed;
            timersub(&now, &oldTime, &elapsed);
            long millisElapsed = ((elapsed.tv_sec * 1000) + (elapsed.tv_usec / 1000));

            if ((timer.tv_sec == 0 && timer.tv_usec == 0) || millisElapsed >= 500) {
                uint32_t time = chromasound->position();

                int minutes = (time / 44100) / 60;
                int seconds = (time / 44100) % 60;

                std::cout << "\r";
                std::cout << std::setfill('0') << std::setw(2) << minutes;
                std::cout << ":";
                std::cout << std::setfill('0') << std::setw(2) << seconds;
                std::cout << " ";
                fflush(stdout);
                timer = now;
            }

            char c;
            if (read(0, &c, 1) == 1) {
                if (c == 'q') {
                    quit = true;
                    break;
                } else if (c == 'p') {
                    quit = false;
                    i -=2;
                    break;
                } else if (c == 'n') {
                    quit = false;
                    break;
                } else if (c == ' ') {
                    if (chromasound->isPaused()) {
                        chromasound->play();
                    } else {
                        chromasound->pause();
                    }
                }
            }
        }

        std::cout << std::endl << std::endl;
        fflush(stdout);
        
        chromasound->stop();

        free(vgm);
        if (vgmFile) fclose(vgmFile);

        if (quit) {
            break;
        }
    }

    if (audioOutput) audioOutput->stop();
    if (audioOutput) audioOutput->destroy();

    delete chromasound;

    tcsetattr(STDIN_FILENO, TCSANOW, &t2);

    return 0;
}
