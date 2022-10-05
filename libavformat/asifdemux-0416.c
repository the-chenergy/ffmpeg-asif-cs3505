/* thu 04/16/20 */

#include "avformat.h"

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
  {
    /* for testing purposes: print out all data in the file */
    if (1 == 1)
    {
      printf("[0]:\t'%c'\n", (char)probe->buf[0]);
      printf("[1]:\t'%c'\n", (char)probe->buf[1]);
      printf("[2]:\t'%c'\n", (char)probe->buf[2]);
      printf("[3]:\t'%c'\n", (char)probe->buf[3]);
      printf("[4]:\t%u\n", (unsigned int)(*(uint32_t*)(&probe->buf[4])));
      printf("[8]:\t%u\n", (unsigned int)(*(uint16_t*)(&probe->buf[8])));
      printf("[10]:\t%u\n", (unsigned int)(*(uint32_t*)(&probe->buf[10])));
      for (int i = 4 + 4 + 2 + 4; i < probe->buf_size; i++)
        printf("[%d]:\t%u\n", i, (unsigned int)probe->buf[i]);
    }

    return AVPROBE_SCORE_MAX;
  }

  return 0;
}

static int asif_read_header(AVFormatContext* context)
{
  printf("------------ asif_read_header() ------------\n");

  /* TODO */

  return 0;
}

static int asif_read_packet(AVFormatContext* context, AVPacket* packet)
{
  printf("------------ asif_read_packet() ------------\n");

  /* TODO */

  return 0;
}

AVInputFormat ff_asif_demuxer = {
    .name = "asif",
    .long_name = NULL_IF_CONFIG_SMALL("Audio Slope Information File"),
    .raw_codec_id = AV_CODEC_ID_ASIF,

    .read_probe = asif_read_probe,
    .read_header = asif_read_header,
    .read_packet = asif_read_packet,
};
