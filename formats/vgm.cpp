#include "vgm.h"

char* VGM::generateHeader(char* dest, size_t dataSize, const int totalSamples, const int loopOffsetData, const int gd3size, const bool selectionLoop)
{
    memset(dest, 0, 128);
    
    // VGM header
    dest[0] = 'V';
    dest[1] = 'g';
    dest[2] = 'm';
    dest[3] = ' ';

    // EOF
    *(uint32_t*)&dest[0x4] = dataSize + gd3size + 128 - 0x4;
    // Version
    dest[0x8] = 0x50;
    dest[0x9] = 0x01;
    // SN76489 clock
    *(uint32_t*)&dest[0xC] = 3579545;
    // YM2413 clock
    *(uint32_t*)&dest[0x10] = 3579545;
    // GD3 offset
    *(uint32_t*)&dest[0x14] = dataSize + 128 - 0x14;
    // Total samples
    *(uint32_t*)&dest[0x18] = totalSamples;
    // Loop offset
    *(uint32_t*)&dest[0x1C] = (loopOffsetData == 0) ? 0 : (loopOffsetData - 0x1C);
    // Loop # samples
    *(uint32_t*)&dest[0x20] = 0;
    // SN76489AN flags
    *(uint16_t*)&dest[0x28] = 0x0003;
    dest[0x2A] = 15;
    // YM2612 clock
    *(uint32_t*)&dest[0x2C] = 7680000;
    // Data offset
    *(uint32_t*)&dest[0x34] = 128 - 0x34;
    // AY8910 clock
    *(uint32_t*)&dest[0x74] = 1789773;

    return dest;
}
