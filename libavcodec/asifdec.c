/**
 * ASIF Decoder
 *
 * Autumrose Stubbs and Qianlang Chen
 * Sun, Apr 19, 2020
 *
 * ASIF stands for "audio slope information file," which encodes audio data in a "delta" format
 * and only captures the slope of an audio wave.
 *
 * Decoder basics:
 * - Packet size: the entire audio.
 * - Frame size: 1 second worth of audio data.
 */

#include "avcodec.h"
#include "internal.h"

/****** STRUCTS *******************************************************************************/

/** The data used throughout the decoding process. */
typedef struct ASIFDecodeData
{
  /** The audio's sampling rate. */
  int sample_rate;

  /** The number of channels in the audio. */
  int num_channels;

  /** The number of samples (per channel) in the audio. */
  int num_samples;

  /** The number of samples (per channel) in the packet that are decoded into frames. */
  int num_read_samples;
} ASIFDecodeData;

/****** HELPER FUNCTIONS **********************************************************************/

/**
 * Decodes a frame size of data (normally 1-second worth) from an input packet (which contains
 * the data of all frames).
 *
 * @param  priv_data  The private data struct used in this encoding process.
 * @param  frame      The frame to put the samples to.
 * @param  source     The input packet's data buffer to read samples from.
 */
static void put_frame_data(ASIFDecodeData* priv_data, AVFrame* frame, uint8_t* source)
{
  uint8_t* channel_source; /* the start of the current channel */

  for (int i = 0; i < priv_data->num_channels; i++)
  {
    channel_source = source + i * priv_data->num_samples + priv_data->num_read_samples;

    for (int j = 0; j < frame->nb_samples; j++)
    {
      if (j == 0 && priv_data->num_read_samples == 0) /* read first samples directly */
      {
        frame->extended_data[i][j] = channel_source[j];
        continue;
      }

      channel_source[j] += channel_source[j - 1]; /* apply delta */
      frame->extended_data[i][j] = channel_source[j];
    }
  }

  priv_data->num_read_samples += frame->nb_samples;
}

/****** DECODER *******************************************************************************/

/**
 * Initializes the decoder and starts the decoding process.
 *
 * @param  context  The codec context.
 */
static int asif_decode_init(AVCodecContext* context)
{
  ASIFDecodeData* data = context->priv_data;

  /* INIT PRIVATE DATA */

  data->sample_rate = context->sample_rate;
  data->num_channels = context->channels;
  data->num_samples = -1; /* unknown until getting a packet in decode_frame() */

  /* INIT NECESSARY CONTEXT PARAMETERS */

  context->sample_fmt = context->codec->sample_fmts[0];

  return 0;
}

/**
 * Decodes a frame worth of audio data from an input packet.
 *
 * @param  context    The codec context.
 * @param  out        A pointer to the current frame.
 * @param  got_frame  Outputs whether a frame was constructed.
 * @param  packet     The input packet to get samples from.
 *
 * @return  0 on success, negative error code on failure.
 */
static int asif_decode_frame(AVCodecContext* context, void* out, int* got_frame,
                             AVPacket* packet)
{
  AVFrame* frame = out;
  ASIFDecodeData* data = context->priv_data;

  int num_remaining_samples = packet->size / data->num_channels - data->num_read_samples;
  int err;

  if (packet->size == 0)
    return 0; /* we assume an empty packet means the whole file has been read, so nothing to do
                 from here */

  /* (MORE) INIT PRIVATE DATA */

  if (data->num_samples == -1)
    data->num_samples = num_remaining_samples;

  /* CONSTRUCT A FRAME */

  frame->nb_samples = context->frame_size;
  if (frame->nb_samples > num_remaining_samples) /* small last frame */
    frame->nb_samples = num_remaining_samples;

  err = ff_get_buffer(context, frame, 0);
  if (err < 0)
    return err;

  /* PUT FRAME DATA */

  put_frame_data(data, frame, packet->data);
  *got_frame = 1;

  if (frame->nb_samples == num_remaining_samples) /* got all frames */
    return packet->size;

  return 0;
}

/** The decoder struct. */
AVCodec ff_asif_decoder = {
    .name = "asif",
    .long_name = NULL_IF_CONFIG_SMALL("Audio Slope Information File"),
    .type = AVMEDIA_TYPE_AUDIO,
    .id = AV_CODEC_ID_ASIF,
    .capabilities = AV_CODEC_CAP_SMALL_LAST_FRAME | AV_CODEC_CAP_SUBFRAMES,
    .sample_fmts = (const enum AVSampleFormat[]){AV_SAMPLE_FMT_U8P, AV_SAMPLE_FMT_NONE},

    .init = asif_decode_init,
    .decode = asif_decode_frame,

    .priv_data_size = sizeof(ASIFDecodeData),
};
