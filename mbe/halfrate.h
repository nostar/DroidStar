/* halfrate: AMBE+2 compatible half-rate (2450 bit/s voice, DMR) speech encoder and decoder.
 *
 * Audio is 8 kHz, 16-bit mono, 160 samples (20 ms) per frame. A frame is 49 bits; the API carries it as 7 bytes,
 * MSB first (the last byte holds one bit, in its top bit). hr_fec_encode / hr_fec_decode turn it into the 9-byte
 * on-air frame with FEC and interleave that DMR carries. hr_bits_from_frame / hr_frame_from_bits convert to and
 * from one bit per byte (on-air order), as other codecs' APIs use.
 *
 * One encoder or decoder per stream: both keep state from frame to frame. Objects are independent and may run on
 * separate threads; one object is not thread-safe.
 */
#ifndef HALFRATE_H
#define HALFRATE_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

#define HR_SAMPLES 160
#define HR_FRAME_BYTES 7
#define HR_AIR_BYTES 9
#define HR_BITS 49

typedef struct hr_encoder hr_encoder;
typedef struct hr_decoder hr_decoder;

hr_encoder *hr_encoder_new(void);
void hr_encoder_free(hr_encoder *e);
/* Sets a tuning option by name (docs/options.md). Returns 0, or -1 for an unknown name or a NaN or infinite
 * value (the option is then left as it was). */
int hr_encoder_set(hr_encoder *e, const char *name, double value);
/* pcm: 160 samples in, frame: 7 bytes out. The output runs HR_ENCODER_DELAY frames behind the input. */
void hr_encode(hr_encoder *e, const int16_t *pcm, uint8_t *frame);
#define HR_ENCODER_DELAY 2

hr_decoder *hr_decoder_new(void);
void hr_decoder_free(hr_decoder *d);
/* Sets an option by name (docs/options.md). Returns 0, or -1 for an unknown name or a NaN or infinite value
 * (the option is then left as it was). */
int hr_decoder_set(hr_decoder *d, const char *name, double value);
/* frame: 7 bytes in, pcm: 160 samples out (must not be null). Returns the frame kind (HR_VOICE, ...), or -1 if d is
 * null.
 * frame may be null for a frame that was not received: it is concealed like an erasure. */
int hr_decode(hr_decoder *d, const uint8_t *frame, int16_t *pcm);
/* Frame kinds hr_decode returns */
#define HR_VOICE 0   /* speech */
#define HR_SILENCE 1 /* a silence frame (background level) */
#define HR_TONE 2    /* a tone frame: single, dual (keypad) or call-progress tone */
#define HR_ERASURE 3 /* not decodable: concealed by repeating the last frame, then muting, then comfort noise */
/* The same on a 9-byte on-air frame: FEC decode, then hr_decode. A null air frame is concealed. */
int hr_decode_air(hr_decoder *d, const uint8_t *air, int16_t *pcm);

/* 7-byte frame <-> 9-byte on-air frame. hr_fec_decode returns the number of bit errors corrected, or -1 when the
 * first code word has more errors than it can correct (the frame is still written, as decoded). The frame helpers'
 * pointers must not be null. */
void hr_fec_encode(const uint8_t *frame, uint8_t *air);
int hr_fec_decode(const uint8_t *air, uint8_t *frame);

/* 7-byte frame <-> 49 bytes of one bit each */
void hr_bits_from_frame(const uint8_t *frame, uint8_t *bits);
void hr_frame_from_bits(const uint8_t *bits, uint8_t *frame);

const char *hr_version(void);

#ifdef __cplusplus
}
#endif
#endif
