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

#include "audioengine.h"
#include <QDebug>
#include <cmath>

#if defined (Q_OS_MACOS) || defined(Q_OS_IOS)
#define MACHAK 1
#else
#define MACHAK 0
#endif

AudioEngine::AudioEngine(QString in, QString out) :
	m_outputdevice(out),
	m_inputdevice(in),
	m_out(nullptr),
	m_in(nullptr),
	m_srm(1)
{
	m_audio_out_temp_buf_p = m_audio_out_temp_buf;
	memset(m_aout_max_buf, 0, sizeof(float) * 200);
	m_aout_max_buf_p = m_aout_max_buf;
	m_aout_max_buf_idx = 0;
	m_aout_gain = 100;
	m_volume = 1.0f;
}

AudioEngine::~AudioEngine()
{
}

QStringList AudioEngine::discover_audio_devices(uint8_t d)
{
	QStringList list;
	QList<QAudioDevice> devices;

	if(d){
		devices = QMediaDevices::audioOutputs();
	}
	else{
		devices = QMediaDevices::audioInputs();
	}

	for (QList<QAudioDevice>::ConstIterator it = devices.constBegin(); it != devices.constEnd(); ++it ) {
		//fprintf(stderr, "Playback device name = %s\n", (*it).deviceName().toStdString().c_str());fflush(stderr);
		list.append((*it).description());
	}

	return list;
}

void AudioEngine::init()
{
	QAudioFormat format;
	format.setSampleRate(8000);
	format.setChannelCount(1);
	format.setSampleFormat(QAudioFormat::Int16);

	m_agc = true;

	QList<QAudioDevice> devices = QMediaDevices::audioOutputs();
	if(devices.size() == 0){
        qDebug() << "No audio playback hardware found";
	}
	else{
		QAudioDevice device(QMediaDevices::defaultAudioOutput());
		for (QList<QAudioDevice>::ConstIterator it = devices.constBegin(); it != devices.constEnd(); ++it ) {

            qDebug() << "Playback device name = " << (*it).description();
            qDebug() << (*it).supportedSampleFormats();
            qDebug() << (*it).preferredFormat();

			if((*it).description() == m_outputdevice){
				device = *it;
			}
		}
		if (!device.isFormatSupported(format)) {
            qWarning() << "Raw audio format not supported by playback device";
        }

        qDebug() << "Playback device: " << device.description() << "SR: " << format.sampleRate();

        m_out = new QAudioSink(device, format, this);
		m_out->setBufferSize(1280);
		connect(m_out, SIGNAL(stateChanged(QAudio::State)), this, SLOT(handleStateChanged(QAudio::State)));
	}

	devices = QMediaDevices::audioInputs();

    if(devices.size() == 0){
        qDebug() <<  "No audio capture hardware found";
	}
	else{
		QAudioDevice device(QMediaDevices::defaultAudioInput());
		for (QList<QAudioDevice>::ConstIterator it = devices.constBegin(); it != devices.constEnd(); ++it ) {
			if(MACHAK){
				qDebug() << "Playback device name = " << (*it).description();
				qDebug() << (*it).supportedSampleFormats();
				qDebug() << (*it).preferredFormat();
			}
			if((*it).description() == m_inputdevice){
				device = *it;
			}
		}
		if (!device.isFormatSupported(format)) {
            qWarning() << "Raw audio format not supported by capture device";
        }

		int sr = 8000;
		if(MACHAK){
			sr = device.preferredFormat().sampleRate();
			m_srm = (float)sr / 8000.0;
		}
		format.setSampleRate(sr);
        m_in = new QAudioSource(device, format, this);
        qDebug() << "Capture device: " <<  device.description() << " SR: " << sr << " resample factor: " << m_srm;
	}
}

void AudioEngine::start_capture()
{
	m_audioinq.clear();
	if(m_in != nullptr){
		m_indev = m_in->start();
		if(MACHAK) m_srm = (float)(m_in->format().sampleRate()) / 8000.0;
		connect(m_indev, SIGNAL(readyRead()), SLOT(input_data_received()));
	}
}

void AudioEngine::stop_capture()
{
	if(m_in != nullptr){
		m_indev->disconnect();
		m_in->stop();
	}
}

void AudioEngine::start_playback()
{
	m_outdev = m_out->start();
}

void AudioEngine::stop_playback()
{
	//m_outdev->reset();
	m_out->reset();
	m_out->stop();
}

void AudioEngine::input_data_received()
{
	QByteArray data = m_indev->readAll();

	if (data.size() > 0){
/*
		fprintf(stderr, "AUDIOIN: ");
		for(int i = 0; i < len; ++i){
			fprintf(stderr, "%02x ", (uint8_t)data.data()[i]);
		}
		fprintf(stderr, "\n");
		fflush(stderr);
*/
		if(MACHAK){
			std::vector<int16_t> samples;
			for(int i = 0; i < data.size(); i += 2){
				samples.push_back(((data.data()[i+1] << 8) & 0xff00) | (data.data()[i] & 0xff));
			}
			for(float i = 0; i < (float)data.size()/2; i += m_srm){
				m_audioinq.enqueue(samples[i]);
			}
		}
		else{
			for(int i = 0; i < data.size(); i += (2 * m_srm)){
				m_audioinq.enqueue(((data.data()[i+1] << 8) & 0xff00) | (data.data()[i] & 0xff));
			}
		}
	}
}

void AudioEngine::write(int16_t *pcm, size_t s)
{
	m_maxlevel = 0;
/*
	fprintf(stderr, "AUDIOOUT: ");
	for(int i = 0; i < s; ++i){
		fprintf(stderr, "%04x ", (uint16_t)pcm[i]);
	}
	fprintf(stderr, "\n");
	fflush(stderr);
*/
	if(m_agc){
		process_audio(pcm, s);
	}

	size_t l = m_outdev->write((const char *) pcm, sizeof(int16_t) * s);

	if (l*2 < s){
		qDebug() << "AudioEngine::write() " << s << ":" << l << ":" << (int)m_out->bytesFree() << ":" << m_out->bufferSize() << ":" << m_out->error();
	}

	for(uint32_t i = 0; i < s; ++i){
		if(pcm[i] > m_maxlevel){
			m_maxlevel = pcm[i];
		}
	}
}

uint16_t AudioEngine::read(int16_t *pcm, int s)
{
	m_maxlevel = 0;

	if(m_audioinq.size() >= s){
		for(int i = 0; i < s; ++i){
			pcm[i] = m_audioinq.dequeue();
			if(pcm[i] > m_maxlevel){
				m_maxlevel = pcm[i];
			}
		}
		return 1;
	}
	else if(m_in == nullptr){
		memset(pcm, 0, sizeof(int16_t) * s);
		return 1;
	}
	else{
		return 0;
	}
}

uint16_t AudioEngine::read(int16_t *pcm)
{
	int s;
	m_maxlevel = 0;

	if(m_audioinq.size() >= 160){
		s = 160;
	}
	else{
		s = m_audioinq.size();
	}

	for(int i = 0; i < s; ++i){
		pcm[i] = m_audioinq.dequeue();
		if(pcm[i] > m_maxlevel){
			m_maxlevel = pcm[i];
		}
	}

	return s;
}

// process_audio() based on code from DSD https://github.com/szechyjs/dsd
void AudioEngine::process_audio(int16_t *pcm, size_t s)
{
	float aout_abs, max, gainfactor, gaindelta, maxbuf;

	for(size_t i = 0; i < s; ++i){
		m_audio_out_temp_buf[i] = static_cast<float>(pcm[i]);
	}

	// detect max level
	max = 0;
	m_audio_out_temp_buf_p = m_audio_out_temp_buf;

	for (size_t i = 0; i < s; i++){
		aout_abs = fabsf(*m_audio_out_temp_buf_p);

		if (aout_abs > max){
			max = aout_abs;
		}

		m_audio_out_temp_buf_p++;
	}

	*m_aout_max_buf_p = max;
	m_aout_max_buf_p++;
	m_aout_max_buf_idx++;

	if (m_aout_max_buf_idx > 24){
		m_aout_max_buf_idx = 0;
		m_aout_max_buf_p = m_aout_max_buf;
	}

	// lookup max history
	for (size_t i = 0; i < 25; i++){
		maxbuf = m_aout_max_buf[i];

		if (maxbuf > max){
			max = maxbuf;
		}
	}

	// determine optimal gain level
	if (max > static_cast<float>(0)){
		gainfactor = (static_cast<float>(30000) / max);
	}
	else{
		gainfactor = static_cast<float>(50);
	}

	if (gainfactor < m_aout_gain){
		m_aout_gain = gainfactor;
		gaindelta = static_cast<float>(0);
	}
	else{
		if (gainfactor > static_cast<float>(50)){
			gainfactor = static_cast<float>(50);
		}

		gaindelta = gainfactor - m_aout_gain;

		if (gaindelta > (static_cast<float>(0.05) * m_aout_gain)){
			gaindelta = (static_cast<float>(0.05) * m_aout_gain);
		}
	}

	gaindelta /= static_cast<float>(s); //160

	// adjust output gain
	m_audio_out_temp_buf_p = m_audio_out_temp_buf;

	for (size_t i = 0; i < s; i++){
		*m_audio_out_temp_buf_p = (m_aout_gain + (static_cast<float>(i) * gaindelta)) * (*m_audio_out_temp_buf_p);
		m_audio_out_temp_buf_p++;
	}

	m_aout_gain += (static_cast<float>(s) * gaindelta);
	m_audio_out_temp_buf_p = m_audio_out_temp_buf;

	for (size_t i = 0; i < s; i++){
		*m_audio_out_temp_buf_p *= m_volume;
		if (*m_audio_out_temp_buf_p > static_cast<float>(32760)){
			*m_audio_out_temp_buf_p = static_cast<float>(32760);
		}
		else if (*m_audio_out_temp_buf_p < static_cast<float>(-32760)){
			*m_audio_out_temp_buf_p = static_cast<float>(-32760);
		}
		pcm[i] = static_cast<int16_t>(*m_audio_out_temp_buf_p);
		m_audio_out_temp_buf_p++;
	}
}

void AudioEngine::handleStateChanged(QAudio::State newState)
{
	switch (newState) {
	case QAudio::ActiveState:
		//qDebug() << "AudioOut state active";
		break;
	case QAudio::SuspendedState:
		//qDebug() << "AudioOut state suspended";
		break;
	case QAudio::IdleState:
		//qDebug() << "AudioOut state idle";
		break;
	case QAudio::StoppedState:
		//qDebug() << "AudioOut state stopped";
		break;
	default:
		break;
	}
}
