#include "gd3.h"

GD3 GD3::parseGd3(const std::string& path)
{
    char* buffer[512];

    FILE* vgmFile = fopen(path.c_str(), "rb");
    
    fseek(vgmFile, 0x14, SEEK_SET);

    uint32_t gd3Offset;
    
    fread((char*)&gd3Offset, sizeof(gd3Offset), 1, vgmFile);

    fseek(vgmFile, gd3Offset + 8, SEEK_CUR);

    char title[128];
    char game[128];
    char author[128];
    char release[128];


    // Title
    int i = 0;
    uint16_t ch = 0xFF;
    while (ch != 0x00) {
        fread(&ch, sizeof(ch), 1, vgmFile);
        title[i] = ch & 0xFF;
        i++;
    }

    ch = 0xFF;
    while (ch != 0x00) {
        fread(&ch, sizeof(ch), 1, vgmFile);
    }

    // Game
    i = 0;
    ch = 0xFF;
    while (ch != 0x00) {
        fread(&ch, sizeof(ch), 1, vgmFile);
        game[i] = ch & 0xFF;
        i++;
    }

    ch = 0xFF;
    while (ch != 0x00) {
        fread(&ch, sizeof(ch), 1, vgmFile);
    }

    // System
    i = 0;
    ch = 0xFF;
    while (ch != 0x00) {
        fread(&ch, sizeof(ch), 1, vgmFile);
    }

    ch = 0xFF;
    while (ch != 0x00) {
        fread(&ch, sizeof(ch), 1, vgmFile);
    }

    // Author
    i = 0;
    ch = 0xFF;
    while (ch != 0x00) {
        fread(&ch, sizeof(ch), 1, vgmFile);
        author[i] = ch & 0xFF;
        i++;
    }

    ch = 0xFF;
    while (ch != 0x00) {
        fread(&ch, sizeof(ch), 1, vgmFile);
    }

    // Release
    i = 0;
    ch = 0xFF;
    while (ch != 0x00) {
        fread(&ch, sizeof(ch), 1, vgmFile);
        release[i] = ch & 0xFF;
        i++;
    }


    GD3 gd3;

    gd3._title = title;
    gd3._game = game;
    gd3._author = author;

    std::string temp = release;
    std::tm t = {};
    std::istringstream ss(temp);
    ss >> std::get_time(&t, "%Y/%m/%d");
    auto to_time_t = std::mktime(&t);
    auto to_time_point = std::chrono::system_clock::from_time_t(to_time_t);
    
    gd3._releaseDate = std::chrono::year_month_day(std::chrono::floor<std::chrono::days>(to_time_point));

    fclose(vgmFile);

    return gd3;
}

const std::string& GD3::title() const
{
    return _title;
}

const std::string& GD3::game() const
{
    return _game;
}

const std::string& GD3::author() const
{
    return _author;
}

const std::chrono::year_month_day& GD3::releaseDate() const
{
    return _releaseDate;
}
