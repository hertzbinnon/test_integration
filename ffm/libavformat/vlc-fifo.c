/*
 * buffered file I/O
 * Copyright (c) 2001 Fabrice Bellard
 *
 * This file is part of Libav.
 *
 * Libav is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * Libav is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Libav; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "libavutil/avstring.h"
#include "libavutil/internal.h"
#include "libavutil/opt.h"
#include "avformat.h"
#include <fcntl.h>
#if HAVE_IO_H
#include <io.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/stat.h>
#include <stdlib.h>
#include "os_support.h"
#include "url.h"
#include "vlc_common.h"
#include "vlc_block.h"


/* standard file protocol */

typedef struct FileContext {
    const AVClass *class;
    int fd;
    int trunc;
    int64_t fifo_num;
    int64_t fifo_size;
    block_fifo_t *fifo;
} FileContext;

static const AVOption fifo_options[] = {
   { "fifo_size", "Truncate existing files on write", offsetof(FileContext, fifo_size), AV_OPT_TYPE_INT, { .i64 = 0 }, 0, 1, AV_OPT_FLAG_ENCODING_PARAM },
    { "fifo_num", "Truncate existing files on write", offsetof(FileContext, fifo_num), AV_OPT_TYPE_INT, { .i64 = 0 }, 0, 0, AV_OPT_FLAG_ENCODING_PARAM },
    { NULL }
};

static const AVClass file_class = {
    .class_name = "fifo",
    .item_name  = av_default_item_name,
    .option     = fifo_options,
    .version    = LIBAVUTIL_VERSION_INT,
};

static int file_read(URLContext *h, unsigned char *buf, int size)
{
    return 0;
}

static int file_write(URLContext *h, const unsigned char *buf, int size)
{
    FileContext *c = h->priv_data;

    block_t *pkt = block_Alloc(size);

    memcpy(pkt->p_buffer,buf,size);
    pkt->i_buffer = size;
    
    vlc_fifo_Lock(c->fifo);
    while (vlc_fifo_GetBytes(c->fifo) + size > c->fifo_size)
    {
        int canc = vlc_savecancel();
        block_Release(vlc_fifo_DequeueUnlocked(c->fifo));
        vlc_restorecancel(canc);
    }
    vlc_fifo_QueueUnlocked(c->fifo, pkt);
    vlc_fifo_Unlock(c->fifo);
    return size;
    //return write(c->fd, buf, size);
}

static int file_open(URLContext *h, const char *filename, int flags)
{
    FileContext *c = h->priv_data;
    int access;
    int fd;

    printf("---> fifo_num = %lld",c->fifo_num);
    printf("---> fifo_addr = %lld",c->fifo);
    printf("---> fifo_size= %lld",c->fifo_size);
    if( !c->fifo_num || !c->fifo_size ) return 1;

    c->fifo = (block_fifo_t*)c->fifo_num;
    printf("---> fifo_num = %lld",c->fifo_num);
    printf("---> fifo_addr = %lld",c->fifo);
    printf("---> fifo_size= %lld",c->fifo_size);

    return 0;
}

static int file_close(URLContext *h)
{
    FileContext *c = h->priv_data;
    return 0;
    //return close(c->fd);
}

URLProtocol ff_file_protocol = {
    .name                = "fifo",
    .url_open            = file_open,
//    .url_read            = file_read,
    .url_write           = file_write,
//    .url_seek            = file_seek,
    .url_close           = file_close,
//    .url_get_file_handle = file_get_handle,
//    .url_check           = file_check,
    .priv_data_size      = sizeof(FileContext),
    .priv_data_class     = &file_class,
};
URLProtocol ff_pipe_protocol = {
    .name                = "pipe",
    .url_open            = file_open,
//    .url_read            = file_read,
    .url_write           = file_write,
//    .url_get_file_handle = file_get_handle,
//    .url_check           = file_check,
    .priv_data_size      = sizeof(FileContext),
};
