#include "avformat.h"

typedef struct
{
  uint32_t sample_rate;
  uint16_t num_channels;
  uint32_t num_samples;
  uint32_t remaining_samples;
} asif_demux_data;

static int asif_read_probe(const AVProbeData* probe)
{
  printf("------------ asif_read_probe() ------------\n");

  /* the file must contain enough bits */
  /* asif <samp-rate (4)> <num-chans (2)> <num-samps (4)> */
  if (probe->buf_size < 4 + 4 + 2 + 4)
    return 0;

  /* the file must start with "asif" */
  if (probe->buf[0] == 'a' && probe->buf[1] == 's' && probe->buf[2] == 'i' &&
      probe->buf[3] == 'f')
    return AVPROBE_SCORE_MAX;

  return 0;
}

static int asif_read_header(AVFormatContext* context)
{
  uint32_t sample_rate = 0;
  uint16_t num_channels = 0;
  uint32_t num_samples = 0;
  AVStream* stream = NULL;
  asif_demux_data* data = context->priv_data;

  printf("------------ asif_read_header() ------------\n");

  /* READ AUDIO DATA */

  avio_skip(context->pb, 4); /* skip header chars "asif" */
  sample_rate = avio_rl32(context->pb);
  num_channels = avio_rl16(context->pb);
  num_samples = avio_rl32(context->pb);
  printf("samp rate: %u\n", sample_rate);
  printf("channels: %u\n", num_channels);
  printf("samples: %u\n", num_samples);

  /* INIT PRIVATE DATA */

  data->sample_rate = sample_rate;
  data->num_channels = num_channels;
  data->num_samples = data->remaining_samples = num_samples;

  /* INIT AUDIO STREAM */

  stream = avformat_new_stream(context, NULL);
  if (!stream) /* out of memory */
    return AVERROR(ENOMEM);

  stream->codecpar->codec_type = AVMEDIA_TYPE_AUDIO;
  stream->codecpar->codec_id = AV_CODEC_ID_ASIF;
  stream->codecpar->sample_rate = sample_rate;
  stream->codecpar->channels = num_channels;
  stream->codecpar->bits_per_coded_sample = 8;
  stream->codecpar->bit_rate = sample_rate * num_channels * 8;
  stream->codecpar->frame_size = num_channels;
  stream->duration = num_samples / sample_rate;

  return 0;
}

static int asif_read_packet(AVFormatContext* context, AVPacket* packet)
{
  int ret = 0;
  asif_demux_data* data = context->priv_data;

  int packet_size = 0;

  printf("------------ asif_read_packet() ------------\n");

  /* TODO: fix this stupid infinite loop!! */

  /* INIT AUDIO PACKET */

  /* define each packet to contain at most one second worth of audio data */
  if (data->remaining_samples < data->sample_rate)
  {
    packet_size = data->remaining_samples * data->num_channels;
    data->remaining_samples = 0;
  }
  else
  {
    packet_size = data->sample_rate * data->num_channels;
    data->remaining_samples -= data->sample_rate;
  }

  printf("rem, rate, pac: %u, %u, %u\n", data->remaining_samples,
         data->sample_rate, packet_size);

  ret = av_get_packet(context->pb, packet, packet_size);
  if (ret < 0) /* error occurred during packet construction */
    return ret;

  return 0;
}

AVInputFormat ff_asif_demuxer = {
    .name = "asif",
    .long_name = NULL_IF_CONFIG_SMALL("Audio Slope Information File"),
    .raw_codec_id = AV_CODEC_ID_ASIF,

    .read_probe = asif_read_probe,
    .read_header = asif_read_header,
    .read_packet = asif_read_packet,

    .priv_data_size = sizeof(asif_demux_data),
};
