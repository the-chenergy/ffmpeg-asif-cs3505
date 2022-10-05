/**
 * ASIF Muxer
 *
 * Autumrose Stubbs and Qianlang Chen
 * Wed, Apr 15, 2020
 *
 * ASIF stands for "audio slope information file," which encodes audio data in a "delta" format
 * and only captures the slope of an audio wave.
 *
 * Muxer basics:
 * - Packet size: the entire audio.
 */

#include "avformat.h"

/****** MUXER *********************************************************************************/

/**
 * Write a packet.
 *
 * @param  context  The format context.
 * @param  packet   The packet to write.
 *
 * @return  0 on success, negative error code on failure.
 */
static int asif_write_packet(AVFormatContext* context, AVPacket* packet)
{
  avio_write(context->pb, packet->data, packet->size);
  return 0;
}

/** The muxer struct. */
AVOutputFormat ff_asif_muxer = {
    .name = "asif",
    .long_name = NULL_IF_CONFIG_SMALL("Audio Slope Information File"),
    .extensions = "asif",
    .audio_codec = AV_CODEC_ID_ASIF,

    .write_packet = asif_write_packet,
};
