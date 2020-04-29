/*****************************************************************************
 * Copyright © 2010-2014 VideoLAN
 *
 * Authors: Jean-Baptiste Kempf <jb@videolan.org>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#include "h264_nal.h"

#include <limits.h>

int convert_sps_pps( decoder_t *p_dec, const uint8_t *p_buf,
                     uint32_t i_buf_size, uint8_t *p_out_buf,
                     uint32_t i_out_buf_size, uint32_t *p_sps_pps_size,
                     uint32_t *p_nal_size)
{
    int i_profile;
    uint32_t i_data_size = i_buf_size, i_nal_size, i_sps_pps_size = 0;
    unsigned int i_loop_end;

    /* */
    if( i_data_size < 7 )
    {
        msg_Err( p_dec, "Input Metadata too small" );
        return VLC_ENOMEM;
    }

    /* Read infos in first 6 bytes */
    i_profile = (p_buf[1] << 16) | (p_buf[2] << 8) | p_buf[3];
    if (p_nal_size)
        *p_nal_size  = (p_buf[4] & 0x03) + 1;
    p_buf       += 5;
    i_data_size -= 5;

    for ( unsigned int j = 0; j < 2; j++ )
    {
        /* First time is SPS, Second is PPS */
        if( i_data_size < 1 )
        {
            msg_Err( p_dec, "PPS too small after processing SPS/PPS %u",
                    i_data_size );
            return VLC_ENOMEM;
        }
        i_loop_end = p_buf[0] & (j == 0 ? 0x1f : 0xff);
        p_buf++; i_data_size--;

        for ( unsigned int i = 0; i < i_loop_end; i++)
        {
            if( i_data_size < 2 )
            {
                msg_Err( p_dec, "SPS is too small %u", i_data_size );
                return VLC_ENOMEM;
            }

            i_nal_size = (p_buf[0] << 8) | p_buf[1];
            p_buf += 2;
            i_data_size -= 2;

            if( i_data_size < i_nal_size )
            {
                msg_Err( p_dec, "SPS size does not match NAL specified size %u",
                        i_data_size );
                return VLC_ENOMEM;
            }
            if( i_sps_pps_size + 4 + i_nal_size > i_out_buf_size )
            {
                msg_Err( p_dec, "Output SPS/PPS buffer too small" );
                return VLC_ENOMEM;
            }

            p_out_buf[i_sps_pps_size++] = 0;
            p_out_buf[i_sps_pps_size++] = 0;
            p_out_buf[i_sps_pps_size++] = 0;
            p_out_buf[i_sps_pps_size++] = 1;

            memcpy( p_out_buf + i_sps_pps_size, p_buf, i_nal_size );
            i_sps_pps_size += i_nal_size;

            p_buf += i_nal_size;
            i_data_size -= i_nal_size;
        }
    }

    *p_sps_pps_size = i_sps_pps_size;

    return VLC_SUCCESS;
}

void convert_h264_to_annexb( uint8_t *p_buf, uint32_t i_len,
                             size_t i_nal_size,
                             struct H264ConvertState *state )
{
    if( i_nal_size < 3 || i_nal_size > 4 )
        return;

    /* This only works for NAL sizes 3-4 */
    while( i_len > 0 )
    {
        if( state->nal_pos < i_nal_size ) {
            unsigned int i;
            for( i = 0; state->nal_pos < i_nal_size && i < i_len; i++, state->nal_pos++ ) {
                state->nal_len = (state->nal_len << 8) | p_buf[i];
                p_buf[i] = 0;
            }
            if( state->nal_pos < i_nal_size )
                return;
            p_buf[i - 1] = 1;
            p_buf += i;
            i_len -= i;
        }
        if( state->nal_len > INT_MAX )
            return;
        if( state->nal_len > i_len )
        {
            state->nal_len -= i_len;
            return;
        }
        else
        {
            p_buf += state->nal_len;
            i_len -= state->nal_len;
            state->nal_len = 0;
            state->nal_pos = 0;
        }
    }
}

bool h264_get_profile_level(const es_format_t *p_fmt, size_t *p_profile,
                            size_t *p_level, size_t *p_nal_size)
{
    uint8_t *p = (uint8_t*)p_fmt->p_extra;
    if(!p || !p_fmt->p_extra) return false;

    /* Check the profile / level */
    if (p_fmt->i_original_fourcc == VLC_FOURCC('a','v','c','1') && p[0] == 1)
    {
        if (p_fmt->i_extra < 12) return false;
        if (p_nal_size) *p_nal_size = 1 + (p[4]&0x03);
        if (!(p[5]&0x1f)) return false;
        p += 8;
    }
    else
    {
        if (p_fmt->i_extra < 8) return false;
        if (!p[0] && !p[1] && !p[2] && p[3] == 1) p += 4;
        else if (!p[0] && !p[1] && p[2] == 1) p += 3;
        else return false;
    }

    if ( ((*p++)&0x1f) != 7) return false;

    /* Get profile/level out of first SPS */
    if (p_profile) *p_profile = p[0];
    if (p_level) *p_level = p[2];
    return true;
}
