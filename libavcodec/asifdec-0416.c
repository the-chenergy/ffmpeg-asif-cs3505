/* wed 04/15/20 */

#include "avcodec.h"

/* TODO */

AVCodec ff_asif_decoder = {
    .name = "asif",
    .long_name = NULL_IF_CONFIG_SMALL("Audio Slope Information File"),
    .type = AVMEDIA_TYPE_AUDIO,
    .id = AV_CODEC_ID_ASIF,

    /* TODO */
};
