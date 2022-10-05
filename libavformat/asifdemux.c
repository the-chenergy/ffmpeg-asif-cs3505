/**
 * ASIF Demuxer
 *
 * Autumrose Stubbs and Qianlang Chen
 * Sun, Apr 19, 2020
 *
 * ASIF stands for "audio slope information file," which encodes audio data in a "delta" format
 * and only captures the slope of an audio wave.
 *
 * Demuxer basics:
 * - Packet size: the entire audio.
 */

#include "avformat.h"
#include "avio.h"
#include "internal.h"

/****** CONSTANTS *****************************************************************************/

/** The size of the portion in the input file that stores audio metadata. */
static const int ASIF_HEADER_SIZE = sizeof(uint32_t) * 3 + sizeof(uint16_t);

/****** DEMUXER *******************************************************************************/

/**
 * Tells from a header if an input file is ASIF format.
 *
 * @param  probe  The input file's probe data.
 *
 * @return  AVPROBE_SCORE_MAX if the file is ASIF format, 0 otherwise.
 */
static int asif_read_probe(const AVProbeData* probe)
{
  if (probe->buf_size >= ASIF_HEADER_SIZE && probe->buf[0] == 'a' && probe->buf[1] == 's' &&
      probe->buf[2] == 'i' && probe->buf[3] == 'f')
    return AVPROBE_SCORE_MAX;

  return 0;
}

/**
 * Reads the header of an ASIF file and starts the demuxing process.
 *
 * @param  context  The codec context.
 *
 * @return  0 on success, negative error code on failure.
 */
static int asif_read_header(AVFormatContext* context)
{
  uint32_t sample_rate;
  uint16_t num_channels;
  uint32_t num_samples;
  int64_t file_body_size;
  AVStream* stream = NULL;

  /* READ AUDIO METADATA */

  avio_skip(context->pb, 4); /* skip header chars "asif" */
  sample_rate = avio_rl32(context->pb);
  num_channels = avio_rl16(context->pb);
  num_samples = avio_rl32(context->pb);
  file_body_size = avio_size(context->pb) - ASIF_HEADER_SIZE;

  /* BASIC VALIDITY CHECKS */

  if (sample_rate == 0)
  {
    av_log(context, AV_LOG_ERROR, "Got zero sample rate.\n\n");
    return AVERROR(EINVAL);
  }

  if (num_channels == 0)
  {
    av_log(context, AV_LOG_ERROR, "Got zero channels.\n\n");
    return AVERROR(EINVAL);
  }

  if (num_samples * num_channels > file_body_size) /* num_samples in file lied */
  {
    av_log(context, AV_LOG_ERROR, "File size (%ld) too small for %u samples.\n\n",
           file_body_size, num_samples);
    return AVERROR(EINVAL);
  }

  if (num_samples * num_channels < file_body_size)
    av_log(context, AV_LOG_WARNING, "File size (%ld) too big for %u samples.\n\n",
           file_body_size, num_samples);

  /* INIT AUDIO STREAM */

  stream = avformat_new_stream(context, NULL);
  if (stream == NULL) /* out of memory */
    return AVERROR(ENOMEM);

  stream->codecpar->codec_type = AVMEDIA_TYPE_AUDIO;
  stream->codecpar->codec_id = AV_CODEC_ID_ASIF;
  stream->codecpar->sample_rate = sample_rate;
  stream->codecpar->channels = num_channels;
  stream->codecpar->bits_per_coded_sample = 8;
  stream->codecpar->bit_rate = sample_rate * num_channels * 8;
  stream->codecpar->frame_size = sample_rate;

  stream->time_base = av_make_q(1, sample_rate);
  stream->duration = num_samples;

  return 0;
}

/**
 * Reads the main body of an ASIF file and construct an input packet.
 *
 * @param  context  The codec context.
 * @param  packet   The input packet to put the file data read in.
 *
 * @return  0 on success, negative error code on failure.
 */
static int asif_read_packet(AVFormatContext* context, AVPacket* packet)
{
  int err = av_get_packet(context->pb, packet, (int)avio_size(context->pb) - ASIF_HEADER_SIZE);
  if (err < 0)
    return err;

  if (packet->size == 0) /* all data has been processed */
    return AVERROR_EOF;

  return 0;
}

/** The demuxer struct. */
AVInputFormat ff_asif_demuxer = {
    .name = "asif",
    .long_name = NULL_IF_CONFIG_SMALL("Audio Slope Information File"),
    .raw_codec_id = AV_CODEC_ID_ASIF,

    .read_probe = asif_read_probe,
    .read_header = asif_read_header,
    .read_packet = asif_read_packet,
};
