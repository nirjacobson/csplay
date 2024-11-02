#ifndef GD3_H
#define GD3_H

#include <string>
#include <chrono>
#include <codecvt>
#include <sstream>

class GD3
{
    public:

        static GD3 parseGd3(const std::string& path);

        const std::string& title() const;
        const std::string& game() const;
        const std::string& author() const;
        const std::chrono::year_month_day& releaseDate() const;

    private:
        std::string _title;
        std::string _game;
        std::string _author;
        std::chrono::year_month_day _releaseDate;
};

#endif // GD3_H
