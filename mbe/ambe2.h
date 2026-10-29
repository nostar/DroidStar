/* ambe2: software AMBE+2 half-rate (DMR) encoder and decoder. GPL-3.0-or-later.
 *
 * The codec is lib/halfrate (its own input filtering, pitch tracking, pause gate, erasure concealment, tones and output
 * limiting; options by name, lib/halfrate/docs/options.md). It writes and plays standard AMBE+2 frames.
 *
 * A frame is 49 bits, one bit per byte (0 or 1), in the on-air order. ambe2_pack/ambe2_unpack turn it into the
 * 9-byte frame with FEC and interleave that DMR carries (36 dibits, first dibit in the top two bits of byte 0).
 * Audio is 8 kHz, 16-bit mono, 160 samples (20 ms) per frame. One encoder or decoder per stream: both keep
 * state from frame to frame. Not thread-safe per object; separate objects are independent.
 */
#ifndef AMBE2_H
#define AMBE2_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

#define AMBE2_SAMPLES 160
#define AMBE2_BITS 49
#define AMBE2_FRAME_BYTES 9

typedef struct ambe2_encoder ambe2_encoder;
typedef struct ambe2_decoder ambe2_decoder;
typedef struct ambe2_stream ambe2_stream;

/* The one codec. The _codec calls take it so that a later codec can sit beside it; any other value gives NULL. */
#define AMBE2_CODEC_HALFRATE 0
/* AMBE2_CODEC_HALFRATE. */
int ambe2_default_codec(void);
/* "halfrate"; NULL for an unknown codec. */
const char *ambe2_codec_name(int codec);
/* The codec of an encoder or decoder. */
int ambe2_encoder_codec(const ambe2_encoder *e);
int ambe2_decoder_codec(const ambe2_decoder *d);

ambe2_encoder *ambe2_encoder_new(void);
/* An encoder of the given codec; NULL for an unknown codec. clipped is not used (kept for source compatibility). */
ambe2_encoder *ambe2_encoder_new_codec(int codec, int clipped);
void ambe2_encoder_free(ambe2_encoder *e);
/* An option of the encoder by name (lib/halfrate/docs/options.md). Returns 0, or -1 for an unknown name or a NaN or
 * infinite value. */
int ambe2_encoder_set_option(ambe2_encoder *e, const char *name, double value);
/* pcm: 160 samples in, bits: 49 bytes out. The encoder looks ahead: its frames come out AMBE2_ENCODER_DELAY frames late
 * (the first ones are silence); ambe2_encode_clip and the stream line them up with the audio. */
void ambe2_encode(ambe2_encoder *e, const int16_t *pcm, uint8_t *bits);
#define AMBE2_ENCODER_DELAY 2
/* A whole clip: x is the raw clip as float samples in [-1, 1] at 8 kHz. n/160 frames of 49 bytes go to bits (room for
 * n/160*49), frame k from samples 160 k to 160 k + 159. Returns the number of frames. The encoder is used up afterwards
 * (free it). */
size_t ambe2_encode_clip(ambe2_encoder *e, const float *x, size_t n, uint8_t *bits);

/* Streaming encoder: raw 8 kHz audio in any chunk size, frames out 40 ms (two frames) late, the same frames as
 * ambe2_encode_clip on the whole clip. One stream per call; separate streams are independent and may run on separate
 * threads. clipped is not used (kept for source compatibility). push returns the frames that became ready (at most
 * max_frames; push 0 samples to fetch the rest); flush ends the stream and returns the frames still waiting. bits: 49
 * bytes per frame. */
ambe2_stream *ambe2_stream_new(int clipped);
/* A stream of the given codec; NULL for an unknown codec. */
ambe2_stream *ambe2_stream_new_codec(int codec, int clipped);
/* ambe2_encoder_set_option on the stream's encoder. Set options before the first push. */
int ambe2_stream_set_option(ambe2_stream *s, const char *name, double value);
void ambe2_stream_free(ambe2_stream *s);
size_t ambe2_stream_push(ambe2_stream *s, const int16_t *pcm, size_t n, uint8_t *bits, size_t max_frames);
size_t ambe2_stream_flush(ambe2_stream *s, uint8_t *bits, size_t max_frames);

/* A new decoder starts from our tuning, not from halfrate's own defaults (kHalfrateDecoderTuning in ambe2.cc: output
 * stage, formant enhancement, voicing mix and a high-band lift on repeated erased frames; docs/tuning.md). It conceals
 * erased frames (repeat, then mute, then comfort noise), plays tone frames and limits its output. */
ambe2_decoder *ambe2_decoder_new(void);
/* A decoder of the given codec; NULL for an unknown codec. */
ambe2_decoder *ambe2_decoder_new_codec(int codec);
void ambe2_decoder_free(ambe2_decoder *d);
/* An option of the decoder by name (lib/halfrate/docs/options.md). Returns 0, or -1 for an unknown name or a NaN or
 * infinite value. */
int ambe2_decoder_set_option(ambe2_decoder *d, const char *name, double value);
/* bits: 49 bytes in, pcm: 160 samples out. Returns 0 for a voice (or silence) frame, 1 for an erased frame, 3 for a tone
 * frame. */
int ambe2_decode(ambe2_decoder *d, const uint8_t *bits, int16_t *pcm);

/* The same calls on 9-byte on-air frames (what DMR carries), for callers that never see the 49-bit form. frames:
 * 9 bytes per frame. The decoder corrects what the FEC can; the return value is as for ambe2_decode. */
size_t ambe2_stream_push_packed(ambe2_stream *s, const int16_t *pcm, size_t n, uint8_t *frames, size_t max_frames);
size_t ambe2_stream_flush_packed(ambe2_stream *s, uint8_t *frames, size_t max_frames);
int ambe2_decode_packed(ambe2_decoder *d, const uint8_t *frame9, int16_t *pcm);

/* Library version and the halfrate codec version, e.g. "2.0.0 halfrate 0.2.0". */
const char *ambe2_version(void);

/* 49 bits <-> the 9-byte on-air frame. unpack returns the number of bit errors the FEC corrected, or -1 when the first
 * code word had more than it can correct (the bits are still written, as decoded). */
void ambe2_pack(const uint8_t *bits, uint8_t *frame9);
int ambe2_unpack(const uint8_t *frame9, uint8_t *bits);

#ifdef __cplusplus
}
#endif
#endif
