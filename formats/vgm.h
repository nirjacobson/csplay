#ifndef VGM_H
#define VGM_H

#include <cstdint>
#include <cstddef>
#include <cstring>

class VGM
{
    public:
        static char* generateHeader(char* dest, size_t dataSize, const int totalSamples, const int loopOffsetData, const int gd3size, const bool selectionLoop);
};

#endif // VGM_H
