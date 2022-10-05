#include "avformat.h"

static int asif_write_packet(AVFormatContext* context, AVPacket* packet)
{
  avio_write(context->pb, packet->data, packet->size);
  return 0;
}

AVOutputFormat ff_asif_muxer = {
    .name = "asif",
    .long_name = NULL_IF_CONFIG_SMALL("Audio Slope Information File"),
    .extensions = "asif",
    .audio_codec = AV_CODEC_ID_ASIF,

    .write_packet = asif_write_packet,
};
