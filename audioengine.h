/*
	Copyright (C) 2019-2021 Doug McLain

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef AUDIOENGINE_H
#define AUDIOENGINE_H

#include <QObject>
#if QT_VERSION < QT_VERSION_CHECK(6, 3, 0)
#include <QAudio>
#include <QAudioFormat>
#include <QAudioInput>
#else
#include <QAudioDevice>
#include <QAudioSink>
#include <QAudioSource>
#include <QMediaDevices>
#endif
#include <QAudioOutput>
#include <QQueue>

#define AUDIO_OUT 1
#define AUDIO_IN  0

class AudioEngine : public QObject
{
	Q_OBJECT
public:
	//explicit AudioEngine(QObject *parent = nullptr);
	AudioEngine(QString in, QString out);
	~AudioEngine();
	static QStringList discover_audio_devices(uint8_t d);
	void init();
	void start_capture();
	void stop_capture();
	void start_playback();
	void stop_playback();
	void write(int16_t *, size_t);
	void set_output_buffer_size(uint32_t b) { m_out->setBufferSize(b); }
	void set_input_buffer_size(uint32_t b) { if(m_in != nullptr) m_in->setBufferSize(b); }
	void set_output_volume(qreal v){ m_out->setVolume(v); }
	void set_input_volume(qreal v){ if(m_in != nullptr) m_in->setVolume(v); }
	void set_agc(bool agc) { m_agc = agc; }
	bool frame_available() { return (m_audioinq.size() >= 320) ? true : false; }
	uint16_t read(int16_t *, int);
	uint16_t read(int16_t *);
	uint16_t level() { return m_maxlevel; }
signals:

private:
	QString m_outputdevice;
	QString m_inputdevice;
#if QT_VERSION < QT_VERSION_CHECK(6, 3, 0)
	QAudioOutput *m_out;
	QAudioInput *m_in;
#else
	QAudioSink *m_out;
	QAudioSource *m_in;
#endif
	QIODevice *m_outdev;
	QIODevice *m_indev;
	QQueue<int16_t> m_audioinq;
	uint16_t m_maxlevel;
	bool m_agc;
	float m_srm; // sample rate multiplier for macOS HACK

	float m_audio_out_temp_buf[320];   //!< output of decoder
	float *m_audio_out_temp_buf_p;

	//float m_audio_out_float_buf[1120]; //!< output of upsampler - 1 frame of 160 samples upampled up to 7 times
	//float *m_audio_out_float_buf_p;

	float m_aout_max_buf[200];
	float *m_aout_max_buf_p;
	int m_aout_max_buf_idx;

	//short m_audio_out_buf[2*48000];    //!< final result - 1s of L+R S16LE samples
	//short *m_audio_out_buf_p;
	//int   m_audio_out_nb_samples;
	//int   m_audio_out_buf_size;
	//int   m_audio_out_idx;
	//int   m_audio_out_idx2;

	float m_aout_gain;
	float m_volume;

private slots:
	void input_data_received();
	void process_audio(int16_t *pcm, size_t s);
	void handleStateChanged(QAudio::State newState);
};

#endif // AUDIOENGINE_H
