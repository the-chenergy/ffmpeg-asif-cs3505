#include "avcodec.h"

static int asif_decode_init(AVCodecContext* context)
{
  printf("------------ asif_decode_init() ------------\n");

  return 0;
}

static int asif_decode_frame(AVCodecContext* context, void* data,
                             int* buffer_size, AVPacket* packet)
{
  printf("------------ asif_decode_frame() ------------\n");

  printf("frame size: %d\n", context->frame_size);
  printf("packet size: %d\n", packet->size);

  for (int i = 0; i < packet->size; i++)
    printf("%u\n", packet->data[i]);

  return packet->size;
}

static int asif_decode_close(AVCodecContext* context)
{
  printf("------------ asif_decode_close() ------------\n");

  return 0;
}

AVCodec ff_asif_decoder = {
    .name = "asif",
    .long_name = NULL_IF_CONFIG_SMALL("ASIF audio file (CS 3505 Spring 20202)"),
    .type = AVMEDIA_TYPE_AUDIO,
    .id = AV_CODEC_ID_ASIF,

    //.init = asif_decode_init,
    .decode = asif_decode_frame,
    //.close = asif_decode_close,
};
