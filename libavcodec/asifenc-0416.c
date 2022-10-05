/**
 * ASIF Encoder
 *
 * Autumrose Stubbs and Qianlang Chen
 * Thu, Apr 16, 2020
 *
 * ASIF stands for "audio slope information file," which encodes audio data in a
 * "delta" format and only captures the slope of an audio wave.
 *
 * Encoder basics:
 * - Frame size: 1 second worth of audio data.
 * - Packet size: the entire audio.
 */

#include "avcodec.h"
#include "bytestream.h"
#include "internal.h"

/****** CONSTANTS *************************************************************/

/** The size of the portion in the output file that stores audio metadata. */
static const int ASIF_HEADER_SIZE = sizeof(uint32_t) * 3 + sizeof(uint16_t);

/** The maximum number of channels supported by the format. */
static const int ASIF_MAX_CHANNELS = UINT16_MAX;

/****** STRUCTS ***************************************************************/

/**
 * Represents a node in a frame linked-list and stores the raw audio data in
 * that frame. The size of a frame is normally 1 second worth of audio data.
 */
typedef struct ASIFEncodeFrame
{
  /** The array of audio data in this frame. */
  uint8_t* frame_data;

  /** The number of elements in the frame_data array. */
  int frame_size;

  /** The next frame (node in the linked-list). */
  struct ASIFEncodeFrame* next_frame;
} ASIFEncodeFrame;

/**
 * The data used throughout the encoding process, including the audio's metadata
 * and a buffer frame list.
 */
typedef struct ASIFEncodeData
{
  /** The audio's sampling rate. */
  uint32_t sample_rate;

  /** The number of channels in the audio. */
  uint16_t num_channels;

  /** The total number of samples in one channel. */
  uint32_t num_samples;

  /** A reference to the start (head node) of the frame linked-list. */
  ASIFEncodeFrame* first_frame;

  /** A reference to the current frame node. */
  ASIFEncodeFrame* curr_frame;

  /**
   * Whether the last frame has been received. This dictates whether we can
   * start preparing the data packet for the muxer.
   */
  int got_last_frame;

  /** Whether the data packet has been made, indicating the end of file. */
  int got_packet;
} ASIFEncodeData;

/****** HELPER FUNCTIONS ******************************************************/

/**
 * Copies the audio data of the current frame into the buffer linked-list.
 *
 * This is used to collect all frames in the audio to later be written to the
 * output packet all at once.
 *
 * @param  priv_data   The private data struct used in this encoding process.
 * @param  frame_data  The audio data of the current frame.
 * @param  frame_size  The number of data samples (for all channels) in the
 *                     current frame.
 */
static void append_frame_data(ASIFEncodeData* priv_data, uint8_t* frame_data,
                              int frame_size)
{
  /* CREATE NEW NODE */

  if (priv_data->curr_frame == NULL) /* no head node yet */
  {
    priv_data->first_frame = av_mallocz(sizeof(ASIFEncodeFrame));
    priv_data->curr_frame = priv_data->first_frame;
  }
  else
  {
    priv_data->curr_frame->next_frame = av_mallocz(sizeof(ASIFEncodeFrame));
    priv_data->curr_frame = priv_data->curr_frame->next_frame;
  }

  /* COPY DATA */

  priv_data->curr_frame->frame_size = frame_size;
  priv_data->curr_frame->frame_data = av_mallocz(frame_size * sizeof(uint8_t));
  for (int i = 0; i < frame_size; i++)
    priv_data->curr_frame->frame_data[i] = frame_data[i];

  /* UPDATE TOTAL SAMPLES */

  priv_data->num_samples += frame_size / priv_data->num_channels;
}

/**
 * Puts the audio data from all frames into an output packet while converting
 * all raw audio data samples into wave slopes.
 *
 * @param  write_dest  The starting point of the data portion in the output
 *                     packet. It should point at somewhere inside the packet's
 *                     data buffer, immediately after where the metadata was
 *                     written.
 * @param  priv_data   The private data struct used in this encoding process.
 */
static void put_frame_data(uint8_t* write_dest, ASIFEncodeData* priv_data)
{
  int* last_sample; /* on each channel */
  int* delta_to_go; /* cumulated delta */
  int curr_channel = 0, curr_sample, delta;

  ASIFEncodeFrame* curr_frame = priv_data->first_frame;

  /* INIT TEMP ARRAYS */

  delta_to_go = av_mallocz(sizeof(int) * priv_data->num_channels);
  last_sample = av_malloc(sizeof(int) * priv_data->num_channels);
  for (int i = 0; i < priv_data->num_channels; i++)
    last_sample[i] = 128; /* 128 to keep the first sample raw (not delta) */

  /* ENCODE AND PUT FRAME DATA INTO PACKET */

  while (curr_frame != NULL)
  {
    for (int i = 0; i < curr_frame->frame_size; i++)
    {
      curr_sample = (int)curr_frame->frame_data[i];
      delta_to_go[curr_channel] += curr_sample - last_sample[curr_channel];

      delta = delta_to_go[curr_channel];
      if (delta < -128) /* no max and min functions in c?? */
        delta = -128;
      else if (delta > 127)
        delta = 127;

      delta_to_go[curr_channel] -= delta;
      last_sample[curr_channel] = curr_sample;

      /* unsigned format unspecified (2's complement or offset??) */
      /* using offset here ([-128..127] => [0..255]) */
      bytestream_put_byte(&write_dest, (unsigned int)(delta + 128));

      curr_channel = (curr_channel + 1) % priv_data->num_channels;
    }

    curr_frame = curr_frame->next_frame;
  }

  /* FREE UP TEMP ARRAYS */

  av_freep(&last_sample);
  av_freep(&delta_to_go);
}

/**
 * Frees up the frame linked-list.
 *
 * @param  priv_data  The private data struct used in this encoding process.
 */
static void free_frame_data(ASIFEncodeData* priv_data)
{
  ASIFEncodeFrame* curr = priv_data->first_frame;
  ASIFEncodeFrame* next;
  while (curr != NULL)
  {
    av_freep(&curr->frame_data);

    next = curr->next_frame;
    av_freep(&curr);
    curr = next;
  }
}

/****** ENCODER ***************************************************************/

/**
 * Initializes the encoder and starts the encoding process.
 *
 * @param  context  The codec context.
 *
 * @return  0 on success, negative error code on failure.
 */
static int asif_encode_init(AVCodecContext* context)
{
  ASIFEncodeData* data = context->priv_data;

  printf("------------ asif_encode_init() ------------\n");

  /* BASIC AUDIO VALIDITY CHECKS */

  if (context->sample_rate < 1)
  {
    av_log(context, AV_LOG_ERROR, "Invalid sample rate: %d\n",
           context->sample_rate);
    return AVERROR(EINVAL);
  }

  if (context->channels < 1 || context->channels > ASIF_MAX_CHANNELS)
  {
    av_log(context, AV_LOG_ERROR,
           "Invalid number of channels: %d (maximum: %d)\n", context->channels,
           ASIF_MAX_CHANNELS);
    return AVERROR(EINVAL);
  }

  /* INIT PRIVATE DATA */

  data->sample_rate = context->sample_rate;
  data->num_channels = context->channels;
  printf("samp rate: %u\n", data->sample_rate);
  printf("channels: %u\n", data->num_channels);

  /* INIT NECESSARY CONTEXT PARAMETERS */

  context->frame_size = data->sample_rate * data->num_channels;

  return 0;
}

/**
 * Retrieves the audio data of a frame.
 *
 * @param  context  The codec context.
 * @param  frame    The current frame input.
 *
 * @return  0 on success, negative error code on failure.
 */
static int asif_send_frame(AVCodecContext* context, const AVFrame* frame)
{
  ASIFEncodeData* data = context->priv_data;

  printf("------------ asif_send_frame() ------------\n");

  /* ffmpeg sends in a null frame for the end of audio */
  if (frame == NULL)
  {
    data->got_last_frame = 1;
    printf("got last frame!! yay!!\n");
  }
  else
  {
    append_frame_data(data, frame->data[0], frame->nb_samples);
    printf("frame size: %d\n", frame->nb_samples);
  }

  return 0;
}

/**
 * Encodes the audio data from all frames into an output packet.
 *
 * @param  context  The codec context.
 * @param  frame    The current frame input.
 *
 * @return  0 on success, AVERROR(EAGAIN) when the last frame is not yet
 *          reached, or other negative error codes on failure.
 */
static int asif_receive_packet(AVCodecContext* context, AVPacket* packet)
{
  ASIFEncodeData* data = context->priv_data;

  int err;
  int64_t packet_size;
  uint8_t* write_dest;

  printf("------------ asif_receive_packet() ------------\n");

  if (data->got_packet) /* the packet is already built */
    return AVERROR_EOF;

  /* KEEP READING UNTIL THE END OF AUDIO */

  if (!data->got_last_frame)
    return AVERROR(EAGAIN); /* request more data/frames */

  /* CONSTRUCT A PACKET */

  printf("samps: %u\n", data->num_samples);

  packet_size = ASIF_HEADER_SIZE + data->num_samples * data->num_channels;
  err = ff_alloc_packet2(context, packet, packet_size, packet_size);
  if (err < 0) /* any error occurred */
    return err;

  /* PUT PACKET DATA */

  write_dest = packet->data;

  bytestream_put_le32(&write_dest, MKTAG('a', 's', 'i', 'f'));
  /* bytestream only supports writing unsigned ints, not uint42_t's */
  bytestream_put_le32(&write_dest, (unsigned int)data->sample_rate);
  bytestream_put_le16(&write_dest, (unsigned int)data->num_channels);
  bytestream_put_le32(&write_dest, (unsigned int)data->num_samples);
  put_frame_data(write_dest, data);

  packet->dts = packet->pts = 1; /* making some up?? */

  data->got_packet = 1;

  return 0;
}

/**
 * Frees up the memory used by the encoder and ends the encoding process.
 *
 * @param  context  The codec context.
 *
 * @return  0 on success, negative error code on failure.
 */
static int asif_encode_close(AVCodecContext* context)
{
  ASIFEncodeData* data = context->priv_data;

  printf("------------ asif_encode_close() ------------\n");

  free_frame_data(data);

  return 0;
}

/** The encoder struct. */
AVCodec ff_asif_encoder = {
    .name = "asif",
    .long_name = NULL_IF_CONFIG_SMALL("Audio Slope Information File"),
    .type = AVMEDIA_TYPE_AUDIO,
    .id = AV_CODEC_ID_ASIF,
    .capabilities = AV_CODEC_CAP_SMALL_LAST_FRAME | AV_CODEC_CAP_DELAY,
    .sample_fmts =
        (const enum AVSampleFormat[]){AV_SAMPLE_FMT_U8, AV_SAMPLE_FMT_NONE},

    .init = asif_encode_init,
    .send_frame = asif_send_frame,
    .receive_packet = asif_receive_packet,
    .close = asif_encode_close,

    .priv_data_size = sizeof(ASIFEncodeData),
};
