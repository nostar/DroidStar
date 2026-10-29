// Public API of ambe2.h over the halfrate codec (lib/halfrate). GPL-3.0-or-later.
#include <string.h>
#include <vector>
#include <stdlib.h>
#include <math.h>
#include "ambe2.h"
#include "halfrate.h"

struct ambe2_encoder { hr_encoder *hr; };
struct ambe2_decoder { hr_decoder *hr; };

int ambe2_default_codec(void) { return AMBE2_CODEC_HALFRATE; }

const char *ambe2_codec_name(int codec) { return codec == AMBE2_CODEC_HALFRATE ? "halfrate" : NULL; }

//const char *ambe2_version(void) { return AMBE2_VERSION " halfrate " HALFRATE_VERSION; }

// 49 bits in on-air order <-> halfrate's 7-byte frame (the same bits, packed MSB first)
static void bits_to_hr(const uint8_t *bits, uint8_t *frame) {
	uint8_t b[AMBE2_BITS];
	for (int i = 0; i < AMBE2_BITS; i++) b[i] = bits[i] & 1;
	hr_frame_from_bits(b, frame);
}

ambe2_encoder *ambe2_encoder_new_codec(int codec, int clipped) {
	if (codec != AMBE2_CODEC_HALFRATE) return NULL;
	ambe2_encoder *e = (ambe2_encoder *)calloc(1, sizeof *e);
	if (!e) return NULL;
	e->hr = hr_encoder_new();
	if (!e->hr) { free(e); return NULL; }
	return e;
}
ambe2_encoder *ambe2_encoder_new(void) { return ambe2_encoder_new_codec(AMBE2_CODEC_HALFRATE, 0); }
void ambe2_encoder_free(ambe2_encoder *e) {
	if (!e) return;
	hr_encoder_free(e->hr);
	free(e);
}
int ambe2_encoder_codec(const ambe2_encoder *e) { return AMBE2_CODEC_HALFRATE; }
int ambe2_encoder_set_option(ambe2_encoder *e, const char *name, double value) { return e ? hr_encoder_set(e->hr, name, value) : -1; }

void ambe2_encode(ambe2_encoder *e, const int16_t *pcm, uint8_t *bits) {
	uint8_t frame[HR_FRAME_BYTES];
	hr_encode(e->hr, pcm, frame);
	hr_bits_from_frame(frame, bits);
}

static void to_pcm(const float *x, int16_t *pcm) {
	for (int i = 0; i < AMBE2_SAMPLES; i++) {
		double v = nearbyint((double)x[i] * 32768.0);
		pcm[i] = (int16_t)(v > 32767 ? 32767 : v < -32768 ? -32768 : v);
	}
}

// Frame k comes out of the encoder AMBE2_ENCODER_DELAY calls later; silence after the clip pushes the last ones out.
size_t ambe2_encode_clip(ambe2_encoder *e, const float *x, size_t n, uint8_t *bits) {
	size_t frames = n / AMBE2_SAMPLES;
	int16_t pcm[AMBE2_SAMPLES];
	uint8_t skip[AMBE2_BITS];
	for (size_t t = 0; t < frames + AMBE2_ENCODER_DELAY; t++) {
		if (t < frames) to_pcm(x + t * AMBE2_SAMPLES, pcm);
		else memset(pcm, 0, sizeof pcm);
		ambe2_encode(e, pcm, t >= AMBE2_ENCODER_DELAY ? bits + (t - AMBE2_ENCODER_DELAY) * AMBE2_BITS : skip);
	}
	return frames;
}

// Streaming: raw samples wait in `pending` until a frame is whole; `encoded` counts encoder calls (with the silence of
// the flush), `real` the frames of audio. The output of call k is frame k - AMBE2_ENCODER_DELAY.
struct ambe2_stream { ambe2_encoder *e; std::vector<int16_t> pending; size_t encoded, real; };

ambe2_stream *ambe2_stream_new_codec(int codec, int clipped) {
	ambe2_encoder *e = ambe2_encoder_new_codec(codec, clipped);
	if (!e) return NULL;
	ambe2_stream *s = new ambe2_stream;
	s->e = e;
	s->encoded = s->real = 0;
	return s;
}
ambe2_stream *ambe2_stream_new(int clipped) { return ambe2_stream_new_codec(AMBE2_CODEC_HALFRATE, clipped); }
void ambe2_stream_free(ambe2_stream *s) { if (s) { ambe2_encoder_free(s->e); delete s; } }
int ambe2_stream_set_option(ambe2_stream *s, const char *name, double value) { return s ? ambe2_encoder_set_option(s->e, name, value) : -1; }

// One encoder call; returns 1 when it gave a frame of the stream (written to bits).
static int stream_encode(ambe2_stream *s, const int16_t *pcm, uint8_t *bits) {
	uint8_t skip[AMBE2_BITS];
	size_t k = s->encoded++;
	bool out = k >= AMBE2_ENCODER_DELAY && k - AMBE2_ENCODER_DELAY < s->real;
	ambe2_encode(s->e, pcm, out ? bits : skip);
	return out;
}

static size_t stream_emit(ambe2_stream *s, uint8_t *bits, size_t max_frames, bool final) {
	size_t out = 0, used = 0;
	// a whole frame of audio is encoded only when its output (if any) has room
	while (s->pending.size() - used >= AMBE2_SAMPLES && (s->encoded < AMBE2_ENCODER_DELAY || out < max_frames)) {
		s->real++;
		out += stream_encode(s, &s->pending[used], bits + out * AMBE2_BITS);
		used += AMBE2_SAMPLES;
	}
	s->pending.erase(s->pending.begin(), s->pending.begin() + used);
	if (final && s->pending.size() < AMBE2_SAMPLES) {   // (whole frames still waiting: no room, call again)
		s->pending.clear();   // a partial last frame is dropped, as in ambe2_encode_clip
		int16_t zero[AMBE2_SAMPLES] = {0};
		while (out < max_frames && s->encoded < s->real + AMBE2_ENCODER_DELAY)
			out += stream_encode(s, zero, bits + out * AMBE2_BITS);
	}
	return out;
}

size_t ambe2_stream_push(ambe2_stream *s, const int16_t *pcm, size_t n, uint8_t *bits, size_t max_frames) {
	s->pending.insert(s->pending.end(), pcm, pcm + n);
	return stream_emit(s, bits, max_frames, false);
}

size_t ambe2_stream_flush(ambe2_stream *s, uint8_t *bits, size_t max_frames) { return stream_emit(s, bits, max_frames, true); }

// halfrate decoder settings fitted against the DVSI stick on the train split of the captured decode clips (2026-10-28),
// by the per-frame distance over 24 mel bands: 2.31 -> 2.01 dB on test, 2.36 -> 2.10 on dev. Most of it is the formant
// enhancement (weaker, split moved from 1 kHz to 460 Hz) and mixed_voiced_share 1. repeat_high_gain lifts the top band
// on erased frames that repeat, as the stick does (about 3 dB per repeat at 3.5 kHz); frames around erasures 2.54 -> 2.04
// dB on test. Its own defaults are in lib/halfrate/docs/options.md; docs/tuning.md has the report.
static const struct { const char *name; double value; } kHalfrateDecoderTuning[] = {
	{"output_gain", 2.07}, {"high_band_tilt", 0.5}, {"high_band_start", 0.66}, {"silence_gain", 0.26},
	{"enhance_strength", 0.61}, {"enhance_min_weight", 0.39}, {"enhance_max_weight", 1.24},
	{"enhance_power_high", 0.72}, {"enhance_power_low", 0.2}, {"enhance_split_frequency", 0.057},
	{"mixed_voiced_share", 1.0}, {"repeat_high_gain", 1.78},
};

ambe2_decoder *ambe2_decoder_new_codec(int codec) {
	if (codec != AMBE2_CODEC_HALFRATE) return NULL;
	ambe2_decoder *d = (ambe2_decoder *)calloc(1, sizeof *d);
	if (!d) return NULL;
	d->hr = hr_decoder_new();
	if (!d->hr) { free(d); return NULL; }
	for (const auto &t : kHalfrateDecoderTuning) hr_decoder_set(d->hr, t.name, t.value);
	return d;
}
ambe2_decoder *ambe2_decoder_new(void) { return ambe2_decoder_new_codec(AMBE2_CODEC_HALFRATE); }
void ambe2_decoder_free(ambe2_decoder *d) {
	if (!d) return;
	hr_decoder_free(d->hr);
	free(d);
}
int ambe2_decoder_codec(const ambe2_decoder *d) { return AMBE2_CODEC_HALFRATE; }
int ambe2_decoder_set_option(ambe2_decoder *d, const char *name, double value) { return d ? hr_decoder_set(d->hr, name, value) : -1; }

// halfrate frame kinds -> the return values of ambe2_decode
static int halfrate_kind(int kind) {
	switch (kind) {
	case HR_ERASURE: return 1;
	case HR_TONE: return 3;
	default: return 0;   // HR_VOICE, HR_SILENCE
	}
}

int ambe2_decode(ambe2_decoder *d, const uint8_t *bits, int16_t *pcm) {
	uint8_t frame[HR_FRAME_BYTES];
	bits_to_hr(bits, frame);
	return halfrate_kind(hr_decode(d->hr, frame, pcm));
}

int ambe2_decode_packed(ambe2_decoder *d, const uint8_t *frame9, int16_t *pcm) { return halfrate_kind(hr_decode_air(d->hr, frame9, pcm)); }

void ambe2_pack(const uint8_t *bits, uint8_t *frame9) {
	uint8_t frame[HR_FRAME_BYTES];
	bits_to_hr(bits, frame);
	hr_fec_encode(frame, frame9);
}

int ambe2_unpack(const uint8_t *frame9, uint8_t *bits) {
	uint8_t frame[HR_FRAME_BYTES];
	int errs = hr_fec_decode(frame9, frame);
	hr_bits_from_frame(frame, bits);
	return errs;
}

static size_t pack_all(const std::vector<uint8_t> &bits, size_t n, uint8_t *frames) {
	for (size_t i = 0; i < n; i++) ambe2_pack(&bits[i * AMBE2_BITS], frames + i * AMBE2_FRAME_BYTES);
	return n;
}

size_t ambe2_stream_push_packed(ambe2_stream *s, const int16_t *pcm, size_t n, uint8_t *frames, size_t max_frames) {
	std::vector<uint8_t> bits(max_frames * AMBE2_BITS + 1);
	return pack_all(bits, ambe2_stream_push(s, pcm, n, bits.data(), max_frames), frames);
}

size_t ambe2_stream_flush_packed(ambe2_stream *s, uint8_t *frames, size_t max_frames) {
	std::vector<uint8_t> bits(max_frames * AMBE2_BITS + 1);
	return pack_all(bits, ambe2_stream_flush(s, bits.data(), max_frames), frames);
}
