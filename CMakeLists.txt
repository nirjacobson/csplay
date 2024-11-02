cmake_minimum_required(VERSION 3.5)

project(csplay VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNODEBUG")

include_directories("/usr/include/glib-2.0")
include_directories("/usr/include/glibmm-2.68")
include_directories("/usr/include/sigc++-3.0")
include_directories("/usr/lib64/glib-2.0/include")
include_directories("/usr/lib64/glibmm-2.68/include")
include_directories("/usr/lib64/sigc++-3.0/include")
include_directories(${CMAKE_SOURCE_DIR})

set(LINUX_ARM OFF)

set(PROJECT_SOURCES
    formats/gd3.cpp
    formats/vgm.cpp
    main.cpp
    chromasound/emu/Vgm_Emu.cc
    chromasound/emu/Blip_Buffer.cc
    chromasound/emu/Gme_File.cc
    chromasound/emu/Classic_Emu.cc
    chromasound/emu/Sms_Apu.cc
    chromasound/emu/Data_Reader.cc
    chromasound/emu/Ym2413_Emu.cc
    chromasound/emu/Multi_Buffer.cc
    chromasound/emu/Effects_Buffer.cc
    chromasound/emu/Dual_Resampler.cc
    chromasound/emu/gme.cc
    chromasound/emu/Vgm_Emu_Impl.cc
    chromasound/emu/Ay_Apu.cc
    chromasound/emu/Ym2612_Emu.cc
    chromasound/emu/Fir_Resampler.cc
    chromasound/emu/M3u_Playlist.cc
    chromasound/emu/Music_Emu.cc
    chromasound/chromasound_dual.cpp
    chromasound/chromasound_direct.cpp
    chromasound/direct/gpio.cpp
    chromasound/direct/vgmplayer.cpp
    chromasound/direct/spi.cpp
    chromasound/chromasound_standin.cpp
    chromasound/chromasound_emu.cpp
)

add_executable(csplay
    ${PROJECT_SOURCES}
)

if (LINUX_ARM)
    target_link_libraries(csplay PRIVATE sigc-3.0 glibmm-2.68 /usr/lib64/libportaudio.so z)
else()
    target_link_libraries(csplay PRIVATE sigc-3.0 glibmm-2.68 /usr/x86_64-pc-linux-gnu/usr/lib64/libportaudio.so z)
endif()

# target_link_libraries(csplay PRIVATE glib-2.0)