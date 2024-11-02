#include <algorithm>
#include <filesystem>
#include <iostream>
#include <limits>
#include <cstdlib>
#include <fcntl.h>
#include <termios.h>

#include "chromasound/emu/audio_output.h"
#include "chromasound/chromasound.h"
#include "chromasound/chromasound_emu.h"
#include "chromasound/chromasound_direct.h"
#include "formats/gd3.h"

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

void usage() {
    std::cout << "csplay" << std::endl;

    std::cout << "Usage: csplay [options] [files]" << std::endl << std::endl;

    std::cout << "Options:" << std::endl;

    std::cout << std::left << std::setw(20) <<  "\t-emu";
    std::cout << "Play using the emulator on the system's default audio output";
    std::cout << std::endl;
    
    std::cout << std::left << std::setw(20) <<  "\t-direct";
    std::cout << "Play using a Chromasound Direct HAT";
    std::cout << std::endl;
}

int main(int argc, char *argv[])
{
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

    if (argc < 2 || access(argv[argc - 1], F_OK) == -1) {
        usage();
        return 0;
    }

    if (cmdOptionExists(argv, argv + argc, "-emu")) {
        chromasound = new Chromasound_Emu;
        audioOutput = AudioOutput<int16_t>::instance();

        freopen("/dev/null", "w", stderr);

        audioOutput->init();
        audioOutput->producer(dynamic_cast<Chromasound_Emu*>(chromasound));

        audioOutput->openDefault(1024);
        audioOutput->start();
        
        i++;
    } else {
        if (cmdOptionExists(argv, argv + argc, "-direct")) {
            i++;
        }
        chromasound = new Chromasound_Direct;
    }

    for (i; i < argc; i++) {
        std::filesystem::path p = argv[i];
        std::string absolutePath = std::filesystem::absolute(p);
        std::string realPath = realpath(absolutePath.c_str(), nullptr);

        GD3 gd3 = GD3::parseGd3(realPath);

        std::cout << gd3.title() << std::endl;
        std::cout << gd3.game() << std::endl;
        std::cout << gd3.author() << std::endl;
        std::cout << gd3.releaseDate() << std::endl << std::endl;

        FILE* file = fopen(realPath.c_str(), "rb");

        fseek(file, 0, SEEK_END);
        size_t size = ftell(file);
        fseek(file, 0, SEEK_SET);

        char* vgm = new char[size];

        size_t bytesRead = 0;
        while (bytesRead < size) {
            bytesRead += fread(vgm + bytesRead, 1, size - bytesRead, file);
        }

        chromasound->play(vgm, size, 0, 0);

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
        fclose(file);

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
