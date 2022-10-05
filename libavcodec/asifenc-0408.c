#include "avcodec.h"
#include "internal.h"

typedef struct
{
} asif_encode_data;

static int asif_encode_init(AVCodecContext* context)
{
  asif_encode_data* data = context->priv_data;

  printf("------------ asif_encode_init() ------------\n");

  printf("channels: %d\n", context->channels);
  printf("samp rate: %d\n", context->sample_rate);

  context->frame_size = context->channels * context->sample_rate;
  printf("frame size: %d\n", context->frame_size);

  return 0;
}

static int asif_encode_frame(AVCodecContext* context, AVPacket* packet,
                             const AVFrame* frame, int* got_packet)
{
  int ret;
  uint8_t* buffer;

  printf("------------ asif_encode_frame() ------------\n");

  printf("frame size: %d\n", context->frame_size);
  printf("line size: %d\n", frame->linesize[0]);
  printf("data: %d\n", frame->extended_data[0]);

  buffer = packet->data;

  *got_packet = 1;

  return 0;
}

static int asif_encode_close(AVCodecContext* context)
{
  return 0;
}

AVCodec ff_asif_encoder = {
    .name = "asif",
    .long_name = NULL_IF_CONFIG_SMALL("Audio Slope Information File"),
    .type = AVMEDIA_TYPE_AUDIO,
    .id = AV_CODEC_ID_ASIF,
    .sample_fmts =
        (const enum AVSampleFormat[]){AV_SAMPLE_FMT_S16, AV_SAMPLE_FMT_NONE},

    .init = asif_encode_init,
    .encode2 = asif_encode_frame,
    .close = asif_encode_close,

    .priv_data_size = sizeof(asif_encode_data),
};
