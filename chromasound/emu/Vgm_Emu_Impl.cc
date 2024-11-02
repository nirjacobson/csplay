// Game_Music_Emu 0.5.5. http://www.slack.net/~ant/

#include "Vgm_Emu.h"

#include <math.h>
#include <string.h>
#include "blargg_endian.h"

/* Copyright (C) 2003-2006 Shay Green. This module is free software; you
can redistribute it and/or modify it under the terms of the GNU Lesser
General Public License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version. This
module is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
details. You should have received a copy of the GNU Lesser General Public
License along with this module; if not, write to the Free Software Foundation,
Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA */

#include "blargg_source.h"

enum {
    cmd_gg_stereo       = 0x4F,
    cmd_psg             = 0x50,
    cmd_ym2413          = 0x51,
    cmd_ym2612_port0    = 0x52,
    cmd_ym2612_port1    = 0x53,
    cmd_ym2151          = 0x54,
    cmd_delay           = 0x61,
    cmd_delay_735       = 0x62,
    cmd_delay_882       = 0x63,
    cmd_byte_delay      = 0x64,
    cmd_end             = 0x66,
    cmd_data_block      = 0x67,
    cmd_short_delay     = 0x70,
    cmd_pcm_delay       = 0x80,
    cmd_pcm_play        = 0x96,
    cmd_pcm_inline      = 0x97,
    cmd_ssg             = 0xA0,
    cmd_pcm_size        = 0xD0,
    cmd_pcm_seek        = 0xE0,
    cmd_pcm_attenuation = 0xF0,

    pcm_block_type      = 0x00,
    ym2612_dac_port     = 0x2A
};

inline int command_len( int command )
{
    switch ( command >> 4 )
    {
        case 0x03:
        case 0x04:
            return 2;

        case 0x05:
        case 0x0A:
        case 0x0B:
            return 3;

        case 0x0C:
        case 0x0D:
            return 4;

        case 0x0E:
        case 0x0F:
            return 5;
    }

    check( false );
    return 1;
}

template<class Emu>
inline void Ym_Emu<Emu>::begin_frame( short* p )
{
    require( enabled() );
    out = p;
    last_time = 0;
}

template<class Emu>
inline int Ym_Emu<Emu>::run_until( int time )
{
    int count = time - last_time;
    if ( count > 0 )
    {
        if ( last_time < 0 )
            return false;
        last_time = time;
        short* p = out;
        out += count * Emu::out_chan_count;
        Emu::run( count, p );
    }
    return true;
}

void Vgm_Emu_Impl::reset()
{
    for (int i = 0; i < PCM_CHANNELS; i++) {
        pcm_att[i] = 0;
        pcm_pos[i] = 0;
        pcm_size[i] = 0;
        pcm_start[i] = 0;
    }
    pcm_discrete = false;
}

inline Vgm_Emu_Impl::fm_time_t Vgm_Emu_Impl::to_fm_time( vgm_time_t t ) const
{
    return (t * fm_time_factor + fm_time_offset) >> fm_time_bits;
}

inline blip_time_t Vgm_Emu_Impl::to_blip_time( vgm_time_t t ) const
{
    return (t * blip_time_factor) >> blip_time_bits;
}

void Vgm_Emu_Impl::write_pcm( vgm_time_t vgm_time, int amp )
{
    blip_time_t blip_time = to_blip_time( vgm_time );
    int old = dac_amp;
    int delta = amp - old;
    dac_amp = amp;
    if ( old >= 0 )
        dac_synth.offset_inline( blip_time, delta, &blip_buf );
    else
        dac_amp |= dac_disabled;
}

void Vgm_Emu_Impl::set_opll_patchset(int patchset)
{
    ym2413.reset_patch( patchset );
}

void Vgm_Emu_Impl::set_pcm_data(const char* path)
{
    Std_File_Reader reader;
    reader.open(path);

    long size = reader.size();
    pcm_data.resize(size);
    reader.read(pcm_data.begin(), size);

    reader.close();
}

void Vgm_Emu_Impl::set_pcm_discrete(bool discrete)
{
    pcm_discrete = discrete;
}

int Vgm_Emu_Impl::pcm_read()
{
    int result = 0x00;

    for (int i = 0; i < PCM_CHANNELS; i++) {
        if (pcm_pos[i]) {
            int sample = *pcm_pos[i] - 0x80;
            pcm_pos[i]++;

            if (pcm_size[i] && pcm_pos[i] == (pcm_start[i] + pcm_size[i])) {
                pcm_pos[i] = 0;
            }
            result += sample >> pcm_att[i];
        }
    }

    result += 0x80;

    if (result < 0)    result = 0;
    if (result > 0xFF) result = 0xFF;

    return result;
}

blip_time_t Vgm_Emu_Impl::run_commands( vgm_time_t end_time )
{
    vgm_time_t vgm_time = this->vgm_time;
    byte const* pos = this->pos;

    if (samples > 0) {
        for (uint32_t i = samples_offset; i < samples && vgm_time < end_time; i++) {
            if (inlinePCMPos) {
                write_pcm( vgm_time, *inlinePCMPos++ );
                vgm_time += 2;
                samples_offset++;
            } else {
                write_pcm( vgm_time++, pcm_read() );
                samples_offset++;
            }
        }
        if (samples_offset == samples) {
            samples = samples_offset = 0;
            inlinePCMPos = NULL;
        }
    } else if ( pos >= data_end ) {
        if (fill_past_end_with_pcm) {
            while (vgm_time < end_time) {
                write_pcm( vgm_time++, pcm_read() );
            }
        } else {
            this->vgm_time = 0;
            return to_blip_time( end_time );
        }
    }

    while ( vgm_time < end_time && pos < data_end )
    {
        // TODO: be sure there are enough bytes left in stream for particular command
        // so we don't read past end
        switch ( *pos++ )
        {
            case cmd_end:
                pos = loop_begin; // if not looped, loop_begin == data_end
                break;

            case cmd_delay_735:
                vgm_time += 735;
                break;

            case cmd_delay_882:
                vgm_time += 882;
                break;

            case cmd_gg_stereo:
                psg.write_ggstereo( to_blip_time( vgm_time ), *pos++ );
                break;

            case cmd_psg:
                psg.write_data( to_blip_time( vgm_time ), *pos++ );
                break;

            case cmd_ssg:
                ssg.write( to_blip_time( vgm_time ), pos[0], pos[1] );
                pos += 2;
                break;

            case cmd_delay:
                vgm_time += pos [1] * 0x100L + pos [0];
                pos += 2;
                break;

            case cmd_byte_delay:
                vgm_time += *pos++;
                break;

            case cmd_ym2413:
                if ( ym2413.run_until( to_fm_time( vgm_time ) ) )
                    ym2413.write( pos [0], pos [1] );
                pos += 2;
                break;

            case cmd_ym2612_port0:
                if ( pos [0] == ym2612_dac_port )
                {
                    write_pcm( vgm_time, pos [1] );
                }
                else if ( ym2612.run_until( to_fm_time( vgm_time ) ) )
                {
                    if ( pos [0] == 0x2B )
                    {
                        if (!pcm_discrete) {
                            dac_disabled = (pos [1] >> 7 & 1) - 1;
                            dac_amp |= dac_disabled;
                            ym2612.write0( pos [0], pos [1] );
                        }
                    } else {
                        ym2612.write0( pos [0], pos [1] );
                    }
                }
                pos += 2;
                break;

            case cmd_ym2612_port1:
                if ( ym2612.run_until( to_fm_time( vgm_time ) ) )
                    ym2612.write1( pos [0], pos [1] );
                pos += 2;
                break;

            case cmd_data_block: {
                check( *pos == cmd_end );
                int type = pos [1];
                long size = GET_LE32( pos + 2 );
                pos += 6;
                pcm_data.resize(size);
                memcpy(pcm_data.begin(), pos, size);
                pos += size;
                break;
            }
            case cmd_pcm_play:
                samples = *(uint32_t*)pos;
                samples_offset = 0;
                inlinePCMPos = NULL;
                for (uint32_t i = 0; i < samples && vgm_time < end_time; i++) {
                    write_pcm( vgm_time++, pcm_read() );
                    samples_offset++;
                }
                pos += 4;
                break;
            case cmd_pcm_inline:
                samples = *(uint32_t*)&pos[1];
                samples_offset = 0;
                inlinePCMPos = &pos[5];
                for (uint32_t i = 0; i < samples && vgm_time < end_time; i++) {
                    write_pcm( vgm_time, pos[5 + i] );
                    vgm_time += 2;
                    samples_offset++;
                }
                pos += 5 + samples;
                break;

            default:
                int cmd = pos [-1];
                int channel;
                uint32_t offset;
                uint8_t att;
                switch ( cmd & 0xF0 )
                {
                    case cmd_pcm_size:
                        channel = (cmd & 0x0F);
                        pcm_size[channel] = *(uint16_t*)pos;
                        pos += 2;
                        break;
                    case cmd_pcm_seek:
                        channel = (cmd & 0x0F);
                        offset = *(uint32_t*)pos;
                        if (offset == -1) {
                            pcm_pos[channel] = 0;
                        } else {
                            pcm_start[channel] = pcm_pos[channel] = pcm_data.begin() + offset;
                        }
                        pos += 4;
                        break;
                    case cmd_pcm_attenuation:
                        channel = (cmd & 0x0F);
                        att = *pos;
                        pcm_att[channel] = att;
                        pos++;
                        break;
                    case cmd_pcm_delay:
                        write_pcm( vgm_time, pcm_read());
                        vgm_time += cmd & 0x0F;
                        break;

                    case cmd_short_delay:
                        vgm_time += (cmd & 0x0F) + 1;
                        break;

                    case 0x50:
                        pos += 2;
                        break;

                    default:
                        pos += command_len( cmd ) - 1;
                        set_warning( "Unknown stream event" );
                }
        }
    }
    if (vgm_time >= end_time)
        vgm_time -= end_time;

    this->pos = pos;
    this->vgm_time = vgm_time;

    return to_blip_time( end_time );
}

int Vgm_Emu_Impl::play_frame( blip_time_t blip_time, int sample_count, sample_t* buf )
{
    // to do: timing is working mostly by luck

    int min_pairs = sample_count >> 1;
    int vgm_time = ((long) min_pairs << fm_time_bits) / fm_time_factor - 1;
    assert( to_fm_time( vgm_time ) <= min_pairs );
    int pairs = min_pairs;
    while ( (pairs = to_fm_time( vgm_time )) < min_pairs )
        vgm_time++;
    //debug_printf( "pairs: %d, min_pairs: %d\n", pairs, min_pairs );

    if ( ym2612.enabled() )
    {
        ym2612.begin_frame( buf );
        memset( buf, 0, pairs * stereo * sizeof *buf );
    }
    else if ( ym2413.enabled() )
    {
        ym2413.begin_frame( buf );
    }

    run_commands( vgm_time );
    ym2612.run_until( pairs );
    ym2413.run_until( pairs );

    fm_time_offset = (vgm_time * fm_time_factor + fm_time_offset) -
                     ((long) pairs << fm_time_bits);

    psg.end_frame( blip_time );
    ssg.end_frame( blip_time );

    return pairs * stereo;
}

// Update pre-1.10 header FM rates by scanning commands
void Vgm_Emu_Impl::update_fm_rates( long* ym2413_rate, long* ym2612_rate ) const
{
    byte const* p = data + 0x40;
    while ( p < data_end )
    {
        switch ( *p )
        {
            case cmd_end:
                return;

            case cmd_psg:
            case cmd_byte_delay:
                p += 2;
                break;

            case cmd_delay:
                p += 3;
                break;

            case cmd_data_block:
                p += 7 + GET_LE32( p + 3 );
                break;

            case cmd_ym2413:
                *ym2612_rate = 0;
                return;

            case cmd_ym2612_port0:
            case cmd_ym2612_port1:
                *ym2612_rate = *ym2413_rate;
                *ym2413_rate = 0;
                return;

            case cmd_ym2151:
                *ym2413_rate = 0;
                *ym2612_rate = 0;
                return;

            default:
                p += command_len( *p );
        }
    }
}
