/*
 * Raw FLAC demuxer
 * Copyright (c) 2001 Fabrice Bellard
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "avformat.h"
#include "flac_picture.h"
#include "internal.h"
#include "libavcodec/flac.h"
#include "oggdec.h"
#include "rawdec.h"
#include "replaygain.h"
#include "vorbiscomment.h"

#define SEEKPOINT_SIZE 18

typedef struct ASIFDecContext
{
  AVClass* class;
  unsigned sample_rate;
  uint16_t channels;
} ASIFDecContext;

static int asif_read_header(AVFormatContext* s)
{
  printf("demuxhahahahahahahahahahahahahahahahahahahahahahahahahahahahahahahaha"
         "hahahahahahahaha\n\n\n\n");
  int ret, metadata_sample_rate = 0, metadata_num_channels,
           metadata_num_samples, found_streaminfo = 0;
  uint8_t header[4];
  uint8_t* buffer = NULL;
  ASIFDecContext* asif = s->priv_data;
  AVStream* st = avformat_new_stream(s, NULL);
  if (!st)
    return AVERROR(ENOMEM);
  st->codecpar->codec_type = AVMEDIA_TYPE_AUDIO;
  st->codecpar->codec_id = AV_CODEC_ID_ASIF;
  st->need_parsing = AVSTREAM_PARSE_FULL_RAW;
  /* the parameters will be extracted from the compressed bitstream */
  st->codecpar->sample_rate = asif->sample_rate;
  st->codecpar->channels = asif->channels;

  printf("%d\n", asif->sample_rate);
  printf("%d\n\n\n\n", st->codecpar->channels);

  for (int i = 0; i < 44; i++)
    printf("%d\t%d\n", i, (uint8_t)(*(&asif + i)));

  return 0;
}

static int joke(AVFormatContext* s)
{
  return 0;
}

FF_RAW_DEMUXER_CLASS(asif)
AVInputFormat ff_asif_demuxer = {
    .name = "asif",
    .long_name = NULL_IF_CONFIG_SMALL("ASIF audio file (CS 3505 Spring 20202)"),
    .read_probe = joke,
    .read_header = asif_read_header,
    .read_packet = joke,
    .read_seek = joke,
    //.read_timestamp = joke,
    .flags = AVFMT_GENERIC_INDEX,
    .extensions = "asif",
    .raw_codec_id = AV_CODEC_ID_ASIF,
    .priv_data_size = sizeof(ASIFDecContext),
    .priv_class = &asif_demuxer_class,
};
