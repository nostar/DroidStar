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

#include "droidstar.h"
#include "httpmanager.h"
#ifdef Q_OS_ANDROID
#include <QtAndroidExtras>
#endif
#ifdef Q_OS_IOS
#include "micpermission.h"
#endif
#include <QStandardPaths>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QFont>
#include <QFontDatabase>
#include <stdio.h>
#include <fcntl.h>
#include <iostream>

DroidStar::DroidStar(QObject *parent) :
	QObject(parent),
	m_dmrid(0),
	m_essid(0),
	m_dmr_destid(0),
	m_outlevel(0),
	m_tts(0)
{
	qRegisterMetaType<Mode::MODEINFO>("Mode::MODEINFO");
	m_settings_processed = false;
	m_modelchange = false;
	connect_status = Mode::DISCONNECTED;
	m_settings = new QSettings(QSettings::IniFormat, QSettings::UserScope, "dudetronics", "droidstar", this);
	config_path = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_WIN)
	config_path += "/dudetronics";
#endif
#if defined(Q_OS_ANDROID)
	keepScreenOn();
	const QString permission("android.permission.READ_EXTERNAL_STORAGE");
	if(QtAndroid::checkPermission(permission) != QtAndroid::PermissionResult::Granted){
		auto resultHash = QtAndroid::requestPermissionsSync(QStringList({permission}));
		if(resultHash[permission] == QtAndroid::PermissionResult::Denied){
			qDebug() << "Storage read permissions denied";
		}
	}
#endif

	check_host_files();
	discover_devices();
	process_settings();

	qDebug() << "CPU arch: " << QSysInfo::currentCpuArchitecture();
	qDebug() << "Build ABI: " << QSysInfo::buildAbi();
	qDebug() << "boot ID: " << QSysInfo::bootUniqueId();
	qDebug() << "Pretty name: " << QSysInfo::prettyProductName();
	qDebug() << "Type: " << QSysInfo::productType();
	qDebug() << "Version: " << QSysInfo::productVersion();
	qDebug() << "Kernel type: " << QSysInfo::kernelType();
	qDebug() << "Kernel version: " << QSysInfo::kernelVersion();
	qDebug() << "Software version: " << VERSION_NUMBER;
}

DroidStar::~DroidStar()
{
}

#ifdef Q_OS_ANDROID
void DroidStar::keepScreenOn()
{
	char const * const action = "addFlags";
	QtAndroid::runOnAndroidThread([action](){
	QAndroidJniObject activity = QtAndroid::androidActivity();
	if (activity.isValid()) {
		QAndroidJniObject window = activity.callObjectMethod("getWindow", "()Landroid/view/Window;");

		if (window.isValid()) {
			const int FLAG_KEEP_SCREEN_ON = 128;
			window.callMethod<void>("addFlags", "(I)V", FLAG_KEEP_SCREEN_ON);
		}
	}});
}

void DroidStar::reset_connect_status()
{
	if(connect_status == Mode::CONNECTED_RW){
		connect_status = Mode::CONNECTING;
		process_connect();
	}
}
#endif

void DroidStar::discover_devices()
{
	m_playbacks.clear();
	m_captures.clear();
	m_vocoders.clear();
	m_modems.clear();
	m_playbacks.append("OS Default");
	m_captures.append("OS Default");
	m_vocoders.append("Software vocoder");
	m_modems.append("None");
	m_playbacks.append(AudioEngine::discover_audio_devices(AUDIO_OUT));
	m_captures.append(AudioEngine::discover_audio_devices(AUDIO_IN));
#if !defined(Q_OS_IOS)
	QMap<QString, QString> l = SerialAMBE::discover_devices();
	QMap<QString, QString>::const_iterator i = l.constBegin();

	while (i != l.constEnd()) {
		m_vocoders.append(i.value());
		m_modems.append(i.value());
		++i;
	}
#endif
}

void DroidStar::download_file(QString f, bool u)
{
	HttpManager *http = new HttpManager(f, u);
	QThread *httpThread = new QThread;
	http->moveToThread(httpThread);
	connect(httpThread, SIGNAL(started()), http, SLOT(process()));
	if(u){
		connect(http, SIGNAL(file_downloaded(QString)), this, SLOT(url_downloaded(QString)));
	}
	else{
		connect(http, SIGNAL(file_downloaded(QString)), this, SLOT(file_downloaded(QString)));
	}
	connect(httpThread, SIGNAL(finished()), http, SLOT(deleteLater()));
	httpThread->start();
}

void DroidStar::url_downloaded(QString url)
{
	emit update_log("Downloaded " + url);
}

void DroidStar::file_downloaded(QString filename)
{
	emit update_log("Updated " + filename);
	{
		if(filename == "dplus.txt" && m_protocol == "REF"){
			process_ref_hosts();
		}
		else if(filename == "dextra.txt" && m_protocol == "XRF"){
			process_xrf_hosts();
		}
		else if(filename == "dcs.txt" && m_protocol == "DCS"){
			process_dcs_hosts();
		}
		else if(filename == "YSFHosts.txt" && m_protocol == "YSF"){
			process_ysf_hosts();
		}
		else if(filename == "FCSHosts.txt" && m_protocol == "FCS"){
			process_fcs_rooms();
		}
		else if(filename == "P25Hosts.txt" && m_protocol == "P25"){
			process_p25_hosts();
		}
		else if(filename == "DMRHosts.txt" && m_protocol == "DMR"){
			process_dmr_hosts();
		}
		else if(filename == "NXDNHosts.txt" && m_protocol == "NXDN"){
			process_nxdn_hosts();
		}
		else if(filename == "M17Hosts-full.csv" && m_protocol == "M17"){
			process_m17_hosts();
		}
		else if(filename == "DMRIDs.dat"){
			process_dmr_ids();
		}
		else if(filename == "NXDN.csv"){
			process_nxdn_ids();
		}
	}
}

void DroidStar::dtmf_send_clicked(QString dtmf)
{
	QByteArray tx(dtmf.simplified().toUtf8(), dtmf.simplified().size());
	//tx.prepend('*');
	emit send_dtmf(tx);
}

void DroidStar::tts_changed(QString tts)
{
	if(tts == "Mic"){
		m_tts = 0;
	}
	else if(tts == "TTS1"){
		m_tts = 1;
	}
	else if(tts == "TTS2"){
		m_tts = 2;
	}
	else if(tts == "TTS3"){
		m_tts = 3;
	}
	else{
		m_tts = 0;
	}
	emit input_source_changed(m_tts, m_ttstxt);
}

void DroidStar::tts_text_changed(QString ttstxt)
{
	m_ttstxt = ttstxt;
	emit input_source_changed(m_tts, m_ttstxt);
}

void DroidStar::process_connect()
{
	if(connect_status != Mode::DISCONNECTED){
		connect_status = Mode::DISCONNECTED;
		m_modethread->quit();
		m_data1.clear();
		m_data2.clear();
		m_data3.clear();
		m_data4.clear();
		m_data5.clear();
		m_data6.clear();
		emit connect_status_changed(0);
		emit update_log("Disconnected");
	}
	else{
#ifdef Q_OS_IOS
		MicPermission::check_permission();
#endif

		if( (m_callsign.size() < 4) ||
			(m_dmrid < 250000) ||
			(m_callsign != m_dmrids[m_dmrid]))
		{
			emit connect_status_changed(4);
			return;
		}

		if(m_protocol == "REF"){
			m_refname = m_saved_refhost;
		}
		else if(m_protocol == "DCS"){
			m_refname = m_saved_dcshost;
		}
		else if(m_protocol == "XRF"){
			m_refname = m_saved_xrfhost;
		}
		else if(m_protocol == "YSF"){
			m_refname = m_saved_ysfhost;
		}
		else if(m_protocol == "FCS"){
			m_refname = m_saved_fcshost;
		}
		else if(m_protocol == "DMR"){
			m_refname = m_saved_dmrhost;
		}
		else if(m_protocol == "P25"){
			m_refname = m_saved_p25host;
		}
		else if(m_protocol == "NXDN"){
			m_refname = m_saved_nxdnhost;
		}
		else if(m_protocol == "M17"){
			m_refname = m_saved_m17host;
		}

		emit connect_status_changed(1);
		connect_status = Mode::CONNECTING;
		QStringList sl;

		if(m_protocol != "IAX"){
			m_host = m_hostmap[m_refname];
			sl = m_host.split(',');

			if( (m_protocol == "M17") && (m_refname != "MMDVM_DIRECT") && (m_ipv6) && (sl.size() > 2) && (sl.at(2) != "none") ){
				m_host = sl.at(2).simplified();
				m_port = sl.at(1).toInt();
			}
			else if(sl.size() > 1){
				m_host = sl.at(0).simplified();
				m_port = sl.at(1).toInt();
			}
			else if( (m_protocol == "M17") && (m_refname == "MMDVM_DIRECT") ){
				qDebug() << "Going MMDVM_DIRECT";
			}
			else{
				m_errortxt = "Invalid host selection";
				connect_status = Mode::DISCONNECTED;
				emit connect_status_changed(5);
				return;
			}
		}

		QString vocoder = "";
		if( (m_vocoder != "Software vocoder") && (m_vocoder.contains(':')) ){
			QStringList vl = m_vocoder.split(':');
			vocoder = vl.at(1);
		}
		QString modem = "";
		if( (m_modem != "None") && (m_modem.contains(':')) ){
			QStringList ml = m_modem.split(':');
			modem = ml.at(1);
		}

		const bool txInvert = true;
		const bool rxInvert = false;
		const bool pttInvert = false;
		const bool useCOSAsLockout = 0;
		const uint32_t ysfTXHang = 4;
		const float pocsagTXLevel = 50;
		const float m17TXLevel = 50;
		const bool duplex = m_modemRxFreq.toUInt() != m_modemTxFreq.toUInt();
		const int rxfreq = m_modemRxFreq.toInt() + m_modemRxOffset.toInt();
		const int txfreq = m_modemTxFreq.toInt() + m_modemTxOffset.toInt();

		emit update_log("Connecting to " + m_host + ":" + QString::number(m_port) + "...");

		m_mode = Mode::create_mode(m_protocol);
		m_modethread = new QThread;
		m_mode->moveToThread(m_modethread);
		m_mode->init(m_callsign, m_dmrid, m_module, m_refname, m_host, m_port, m_ipv6, vocoder, modem, m_capture, m_playback);
		m_mode->set_modem_flags(rxInvert, txInvert, pttInvert, useCOSAsLockout, duplex);
		m_mode->set_modem_params(m_modemBaud.toUInt(), rxfreq, txfreq, m_modemTxDelay.toInt(), m_modemRxLevel.toFloat(), m_modemRFLevel.toFloat(), ysfTXHang, m_modemCWIdTxLevel.toFloat(), m_modemDstarTxLevel.toFloat(), m_modemDMRTxLevel.toFloat(), m_modemYSFTxLevel.toFloat(), m_modemP25TxLevel.toFloat(), m_modemNXDNTxLevel.toFloat(), pocsagTXLevel, m17TXLevel);

		connect(this, SIGNAL(module_changed(char)), m_mode, SLOT(module_changed(char)));
		connect(m_mode, SIGNAL(update(Mode::MODEINFO)), this, SLOT(update_data(Mode::MODEINFO)));
		connect(m_mode, SIGNAL(update_output_level(unsigned short)), this, SLOT(update_output_level(unsigned short)));
		connect(m_modethread, SIGNAL(started()), m_mode, SLOT(send_connect()));
		connect(m_modethread, SIGNAL(finished()), m_mode, SLOT(deleteLater()));
		connect(this, SIGNAL(input_source_changed(int,QString)), m_mode, SLOT(input_src_changed(int,QString)));
		connect(this, SIGNAL(swrx_state_changed(int)), m_mode, SLOT(swrx_state_changed(int)));
		connect(this, SIGNAL(swtx_state_changed(int)), m_mode, SLOT(swtx_state_changed(int)));
		connect(this, SIGNAL(agc_state_changed(int)), m_mode, SLOT(agc_state_changed(int)));
		connect(this, SIGNAL(tx_clicked(bool)), m_mode, SLOT(toggle_tx(bool)));
		connect(this, SIGNAL(tx_pressed()), m_mode, SLOT(start_tx()));
		connect(this, SIGNAL(tx_released()), m_mode, SLOT(stop_tx()));
		connect(this, SIGNAL(in_audio_vol_changed(qreal)), m_mode, SLOT(in_audio_vol_changed(qreal)));
		connect(this, SIGNAL(mycall_changed(QString)), m_mode, SLOT(mycall_changed(QString)));
		connect(this, SIGNAL(urcall_changed(QString)), m_mode, SLOT(urcall_changed(QString)));
		connect(this, SIGNAL(rptr1_changed(QString)), m_mode, SLOT(rptr1_changed(QString)));
		connect(this, SIGNAL(rptr2_changed(QString)), m_mode, SLOT(rptr2_changed(QString)));
		connect(this, SIGNAL(usrtxt_changed(QString)), m_mode, SLOT(usrtxt_changed(QString)));
		emit module_changed(m_module);
		emit mycall_changed(m_mycall);
		emit urcall_changed(m_urcall);
		emit rptr1_changed(m_rptr1);
		emit rptr2_changed(m_rptr2);
		emit usrtxt_changed(m_dstarusertxt);

		if(m_protocol == "DMR"){
			QString dmrpass = sl.at(2).simplified();

			if((m_refname.size() > 2) && (m_refname.left(2) == "BM")){
				if(!m_bm_password.isEmpty()){
					dmrpass = m_bm_password;
				}
			}

			if((m_refname.size() > 4) && (m_refname.left(4) == "TGIF")){
				if(!m_tgif_password.isEmpty()){
					dmrpass = m_tgif_password;
				}
			}
			m_mode->set_dmr_params(m_essid, dmrpass, m_latitude, m_longitude, m_location, m_description, m_freq, m_url, m_swid, m_pkgid, m_dmropts);
			connect(this, SIGNAL(dmr_tgid_changed(int)), m_mode, SLOT(dmr_tgid_changed(int)));
			connect(this, SIGNAL(dmrpc_state_changed(int)), m_mode, SLOT(dmrpc_state_changed(int)));
			connect(this, SIGNAL(slot_changed(int)), m_mode, SLOT(slot_changed(int)));
			connect(this, SIGNAL(cc_changed(int)), m_mode, SLOT(cc_changed(int)));
		}

		if(m_protocol == "M17"){
			connect(this, SIGNAL(m17_rate_changed(int)), m_mode, SLOT(rate_changed(int)));
			connect(this, SIGNAL(m17_can_changed(int)), m_mode, SLOT(can_changed(int)));
		}

		if(m_protocol == "IAX"){
			m_mode->set_iax_params(m_iaxuser, m_iaxpassword, m_iaxnode, m_iaxhost, m_iaxport);
			connect(this, SIGNAL(send_dtmf(QByteArray)), m_mode, SLOT(send_dtmf(QByteArray)));
		}

		m_modethread->start();

	}

	qDebug() << "process_connect called m_callsign == " << m_callsign;
	qDebug() << "process_connect called m_dmrid == " << m_dmrid;
	qDebug() << "process_connect called m_bm_password == " << m_bm_password;
	qDebug() << "process_connect called m_tgif_password == " << m_tgif_password;
	qDebug() << "process_connect called m_dmropts == " << m_dmropts;
	qDebug() << "process_connect called m_refname == " << m_refname;
	qDebug() << "process_connect called m_host == " << m_host;
	qDebug() << "process_connect called m_module == " << m_module;
	qDebug() << "process_connect called m_protocol == " << m_protocol;
	qDebug() << "process_connect called m_port == " << m_port;
}

void DroidStar::process_host_change(const QString &h)
{
	if(m_protocol == "REF"){
		m_saved_refhost = h.simplified();
	}
	if(m_protocol == "DCS"){
		m_saved_dcshost = h.simplified();
	}
	if(m_protocol == "XRF"){
		m_saved_xrfhost = h.simplified();
	}
	if(m_protocol == "YSF"){
		m_saved_ysfhost = h.simplified();
	}
	if(m_protocol == "FCS"){
		m_saved_fcshost = h.simplified();
	}
	if(m_protocol == "DMR"){
		m_saved_dmrhost = h.simplified();
	}
	if(m_protocol == "P25"){
		m_saved_p25host = h.simplified();
	}
	if(m_protocol == "NXDN"){
		m_saved_nxdnhost = h.simplified();
	}
	if(m_protocol == "M17"){
		m_saved_m17host = h.simplified();
	}
	if(m_protocol == "IAX"){
		m_saved_iaxhost = h.simplified();
	}
	save_settings();
}

void DroidStar::process_mode_change(const QString &m)
{
	m_protocol = m;
	if(m == "REF"){
		process_ref_hosts();
		m_label1 = "MYCALL";
		m_label2 = "URCALL";
		m_label3 = "RPTR1";
		m_label4 = "RPTR2";
		m_label5 = "Stream ID";
		m_label6 = "User txt";
	}
	if(m == "DCS"){
		process_dcs_hosts();
		m_label1 = "MYCALL";
		m_label2 = "URCALL";
		m_label3 = "RPTR1";
		m_label4 = "RPTR2";
		m_label5 = "Stream ID";
		m_label6 = "User txt";
	}
	if(m == "XRF"){
		process_xrf_hosts();
		m_label1 = "MYCALL";
		m_label2 = "URCALL";
		m_label3 = "RPTR1";
		m_label4 = "RPTR2";
		m_label5 = "Stream ID";
		m_label6 = "User txt";
	}
	if(m == "YSF"){
		process_ysf_hosts();
		m_label1 = "Gateway";
		m_label2 = "Callsign";
		m_label3 = "Dest";
		m_label4 = "Type";
		m_label5 = "Path";
		m_label6 = "Frame#";
	}
	if(m == "FCS"){
		process_fcs_rooms();
		m_label1 = "Gateway";
		m_label2 = "Callsign";
		m_label3 = "Dest";
		m_label4 = "Type";
		m_label5 = "Path";
		m_label6 = "Frame#";
	}
	if(m == "DMR"){
		process_dmr_hosts();
		//process_dmr_ids();
		m_label1 = "Callsign";
		m_label2 = "SrcID";
		m_label3 = "DestID";
		m_label4 = "GWID";
		m_label5 = "Info";
		m_label6 = "";
	}
	if(m == "P25"){
		process_p25_hosts();
		m_label1 = "Callsign";
		m_label2 = "SrcID";
		m_label3 = "DestID";
		m_label4 = "GWID";
		m_label5 = "Seq#";
		m_label6 = "";
	}
	if(m == "NXDN"){
		process_nxdn_hosts();
		m_label1 = "Callsign";
		m_label2 = "SrcID";
		m_label3 = "DestID";
		m_label4 = "GWID";
		m_label5 = "Seq#";
		m_label6 = "";
	}
	if(m == "M17"){
		process_m17_hosts();
		m_label1 = "SrcID";
		m_label2 = "DstID";
		m_label3 = "Type";
		m_label4 = "Frame#";
		m_label5 = "StreamID";
		m_label6 = "";
	}
	if(m == "IAX"){
		m_label1 = "";
		m_label2 = "";
		m_label3 = "";
		m_label4 = "";
		m_label5 = "";
		m_label6 = "";
	}
	emit mode_changed();
}

void DroidStar::save_settings()
{
	//m_settings->setValue("PLAYBACK", ui->comboPlayback->currentText());
	//m_settings->setValue("CAPTURE", ui->comboCapture->currentText());
	m_settings->setValue("IPV6", m_ipv6 ? "true" : "false");
	m_settings->setValue("MODE", m_protocol);
	m_settings->setValue("REFHOST", m_saved_refhost);
	m_settings->setValue("DCSHOST", m_saved_dcshost);
	m_settings->setValue("XRFHOST", m_saved_xrfhost);
	m_settings->setValue("YSFHOST", m_saved_ysfhost);
	m_settings->setValue("FCSHOST", m_saved_fcshost);
	m_settings->setValue("DMRHOST", m_saved_dmrhost);
	m_settings->setValue("P25HOST", m_saved_p25host);
	m_settings->setValue("NXDNHOST", m_saved_nxdnhost);
	m_settings->setValue("M17HOST", m_saved_m17host);
	m_settings->setValue("MODULE", QString(m_module));
	m_settings->setValue("CALLSIGN", m_callsign);
	m_settings->setValue("DMRID", m_dmrid);
	m_settings->setValue("ESSID", m_essid);
	m_settings->setValue("BMPASSWORD", m_bm_password);
	m_settings->setValue("TGIFPASSWORD", m_tgif_password);
	m_settings->setValue("DMRTGID", m_dmr_destid);
	m_settings->setValue("DMRLAT", m_latitude);
	m_settings->setValue("DMRLONG", m_longitude);
	m_settings->setValue("DMRLOC", m_location);
	m_settings->setValue("DMRDESC", m_description);
	m_settings->setValue("DMRFREQ", m_freq);
	m_settings->setValue("DMRURL", m_url);
	m_settings->setValue("DMRSWID", m_swid);
	m_settings->setValue("DMRPKGID", m_pkgid);
	m_settings->setValue("DMROPTS", m_dmropts);
	m_settings->setValue("MYCALL", m_mycall);
	m_settings->setValue("URCALL", m_urcall);
	m_settings->setValue("RPTR1", m_rptr1);
	m_settings->setValue("RPTR2", m_rptr2);
	m_settings->setValue("TXTIMEOUT", m_txtimeout);
	m_settings->setValue("TXTOGGLE", m_toggletx ? "true" : "false");
	m_settings->setValue("XRF2REF", m_xrf2ref ? "true" : "false");
	m_settings->setValue("USRTXT", m_dstarusertxt);
	m_settings->setValue("IAXUSER", m_iaxuser);
	m_settings->setValue("IAXPASS", m_iaxpassword);
	m_settings->setValue("IAXNODE", m_iaxnode);
	m_settings->setValue("IAXHOST", m_iaxhost);
	m_settings->setValue("IAXPORT", m_iaxport);

	m_settings->setValue("ModemRxFreq", m_modemRxFreq);
	m_settings->setValue("ModemTxFreq", m_modemTxFreq);
	m_settings->setValue("ModemRxOffset", m_modemRxOffset);
	m_settings->setValue("ModemTxOffset", m_modemTxOffset);
	m_settings->setValue("ModemRxDCOffset", m_modemRxDCOffset);
	m_settings->setValue("ModemTxDCOffset", m_modemTxDCOffset);
	m_settings->setValue("ModemRxLevel", m_modemRxLevel);
	m_settings->setValue("ModemTxLevel", m_modemTxLevel);
	m_settings->setValue("ModemRFLevel", m_modemRFLevel);
	m_settings->setValue("ModemTxDelay", m_modemTxDelay);
	m_settings->setValue("ModemCWIdTxLevel", m_modemCWIdTxLevel);
	m_settings->setValue("ModemDstarTxLevel", m_modemDstarTxLevel);
	m_settings->setValue("ModemDMRTxLevel", m_modemDMRTxLevel);
	m_settings->setValue("ModemYSFTxLevel", m_modemYSFTxLevel);
	m_settings->setValue("ModemP25TxLevel", m_modemP25TxLevel);
	m_settings->setValue("ModemNXDNTxLevel", m_modemNXDNTxLevel);
	m_settings->setValue("ModemBaud", m_modemBaud);
	m_settings->setValue("ModemM17CAN", m_modemM17CAN);
	m_settings->setValue("ModemTxInvert", m_modemTxInvert ? "true" : "false");
	m_settings->setValue("ModemRxInvert", m_modemRxInvert ? "true" : "false");
	m_settings->setValue("ModemPTTInvert", m_modemPTTInvert ? "true" : "false");
}

void DroidStar::process_settings()
{
	m_ipv6 = (m_settings->value("IPV6").toString().simplified() == "true") ? true : false;
	process_mode_change(m_settings->value("MODE").toString().simplified());
	m_saved_refhost = m_settings->value("REFHOST").toString().simplified();
	m_saved_dcshost =m_settings->value("DCSHOST").toString().simplified();
	m_saved_xrfhost = m_settings->value("XRFHOST").toString().simplified();
	m_saved_ysfhost = m_settings->value("YSFHOST").toString().simplified();
	m_saved_fcshost = m_settings->value("FCSHOST").toString().simplified();
	m_saved_dmrhost = m_settings->value("DMRHOST").toString().simplified();
	m_saved_p25host = m_settings->value("P25HOST").toString().simplified();
	m_saved_nxdnhost = m_settings->value("NXDNHOST").toString().simplified();
	m_saved_m17host = m_settings->value("M17HOST").toString().simplified();
	m_module = m_settings->value("MODULE").toString().toStdString()[0];
	m_callsign = m_settings->value("CALLSIGN").toString().simplified();
	m_dmrid = m_settings->value("DMRID").toString().simplified().toUInt();
	m_essid = m_settings->value("ESSID").toString().simplified().toUInt();
	m_bm_password = m_settings->value("BMPASSWORD").toString().simplified();
	m_tgif_password = m_settings->value("TGIFPASSWORD").toString().simplified();
	m_latitude = m_settings->value("DMRLAT", "0").toString().simplified();
	m_longitude = m_settings->value("DMRLONG", "0").toString().simplified();
	m_location = m_settings->value("DMRLOC").toString().simplified();
	m_description = m_settings->value("DMRDESC", "DroidStar").toString().simplified();
	m_freq = m_settings->value("DMRFREQ", "438800000").toString().simplified();
	m_url = m_settings->value("DMRURL", "www.qrz.com").toString().simplified();
	m_swid = m_settings->value("DMRSWID", "20200922").toString().simplified();
	m_pkgid = m_settings->value("DMRPKGID", "MMDVM_MMDVM_HS_Hat").toString().simplified();
	m_dmropts = m_settings->value("DMROPTS").toString().simplified();
	m_dmr_destid = m_settings->value("DMRTGID", "4000").toString().simplified().toUInt();
	m_mycall = m_settings->value("MYCALL").toString().simplified();
	m_urcall = m_settings->value("URCALL", "CQCQCQ").toString().simplified();
	m_rptr1 = m_settings->value("RPTR1").toString().simplified();
	m_rptr2 = m_settings->value("RPTR2").toString().simplified();
	m_txtimeout = m_settings->value("TXTIMEOUT", "300").toString().simplified().toUInt();
	m_toggletx = (m_settings->value("TXTOGGLE", "true").toString().simplified() == "true") ? true : false;
	m_dstarusertxt = m_settings->value("USRTXT").toString().simplified();
	m_xrf2ref = (m_settings->value("XRF2REF").toString().simplified() == "true") ? true : false;
	m_iaxuser = m_settings->value("IAXUSER").toString().simplified();
	m_iaxpassword = m_settings->value("IAXPASS").toString().simplified();
	m_iaxnode = m_settings->value("IAXNODE").toString().simplified();
	m_saved_iaxhost = m_iaxhost = m_settings->value("IAXHOST").toString().simplified();
	m_iaxport = m_settings->value("IAXPORT", "4569").toString().simplified().toUInt();
	m_localhosts = m_settings->value("LOCALHOSTS").toString();

	m_modemRxFreq = m_settings->value("ModemRxFreq", "438800000").toString().simplified();
	m_modemTxFreq = m_settings->value("ModemTxFreq", "438800000").toString().simplified();
	m_modemRxOffset = m_settings->value("ModemRxOffset", "0").toString().simplified();
	m_modemTxOffset = m_settings->value("ModemTxOffset", "0").toString().simplified();
	m_modemRxDCOffset = m_settings->value("ModemRxDCOffset", "0").toString().simplified();
	m_modemTxDCOffset = m_settings->value("ModemTxDCOffset", "0").toString().simplified();
	m_modemRxLevel = m_settings->value("ModemRxLevel", "50").toString().simplified();
	m_modemTxLevel = m_settings->value("ModemTxLevel", "50").toString().simplified();
	m_modemRFLevel = m_settings->value("ModemRFLevel", "100").toString().simplified();
	m_modemTxDelay = m_settings->value("ModemTxDelay", "100").toString().simplified();
	m_modemCWIdTxLevel = m_settings->value("ModemCWIdTxLevel", "50").toString().simplified();
	m_modemDstarTxLevel = m_settings->value("ModemDstarTxLevel", "50").toString().simplified();
	m_modemDMRTxLevel = m_settings->value("ModemDMRTxLevel", "50").toString().simplified();
	m_modemYSFTxLevel = m_settings->value("ModemYSFTxLevel", "50").toString().simplified();
	m_modemP25TxLevel = m_settings->value("ModemP25TxLevel", "50").toString().simplified();
	m_modemNXDNTxLevel = m_settings->value("ModemNXDNTxLevel", "50").toString().simplified();
	m_modemBaud = m_settings->value("ModemBaud", "115200").toString().simplified();
	m_modemM17CAN = m_settings->value("ModemM17CAN", "0").toString().simplified();
	m_modemTxInvert = (m_settings->value("ModemTxInvert", "true").toString().simplified() == "true") ? true : false;
	m_modemRxInvert = (m_settings->value("ModemRxInvert", "false").toString().simplified() == "true") ? true : false;
	m_modemPTTInvert = (m_settings->value("ModemPTTInvert", "false").toString().simplified() == "true") ? true : false;
	emit update_settings();
}

void DroidStar::update_custom_hosts(QString h)
{
	m_settings->setValue("LOCALHOSTS", h);
	m_localhosts = m_settings->value("LOCALHOSTS").toString();
}

void DroidStar::process_ref_hosts()
{
	m_hostmap.clear();
	m_hostsmodel.clear();
	QFileInfo check_file(config_path + "/dplus.txt");

	if(check_file.exists() && check_file.isFile()){
		QFile f(config_path + "/dplus.txt");
		if(f.open(QIODevice::ReadOnly)){
			while(!f.atEnd()){
				QString l = f.readLine();
				if(l.at(0) == '#'){
					continue;
				}
				QStringList ll = l.split('\t');
				if(ll.size() > 1){
					m_hostmap[ll.at(0).simplified()] = ll.at(1).simplified() + ",20001";
				}
			}

			m_customhosts = m_localhosts.split('\n');
			for (const auto& i : m_customhosts){
				QStringList line = i.simplified().split(' ');

				if(line.at(0) == "REF"){
					m_hostmap[line.at(1).simplified()] = line.at(2).simplified() + "," + line.at(3).simplified();
				}
			}

			QMap<QString, QString>::const_iterator i = m_hostmap.constBegin();
			while (i != m_hostmap.constEnd()) {
				m_hostsmodel.append(i.key());
				++i;
			}
		}
		f.close();
	}
	else{
		download_file("/dplus.txt");
	}
}

void DroidStar::process_dcs_hosts()
{
	m_hostmap.clear();
	m_hostsmodel.clear();
	QFileInfo check_file(config_path + "/dcs.txt");
	if(check_file.exists() && check_file.isFile()){
		QFile f(config_path + "/dcs.txt");
		if(f.open(QIODevice::ReadOnly)){
			while(!f.atEnd()){
				QString l = f.readLine();
				if(l.at(0) == '#'){
					continue;
				}
				QStringList ll = l.split('\t');
				if(ll.size() > 1){
					m_hostmap[ll.at(0).simplified()] = ll.at(1).simplified() + ",30051";
				}
			}

			m_customhosts = m_localhosts.split('\n');
			for (const auto& i : m_customhosts){
				QStringList line = i.simplified().split(' ');

				if(line.at(0) == "DCS"){
					m_hostmap[line.at(1).simplified()] = line.at(2).simplified() + "," + line.at(3).simplified();
				}
			}

			QMap<QString, QString>::const_iterator i = m_hostmap.constBegin();
			while (i != m_hostmap.constEnd()) {
				m_hostsmodel.append(i.key());
				++i;
			}
		}
		f.close();
	}
	else{
		download_file("/dcs.txt");
	}
}

void DroidStar::process_xrf_hosts()
{
	m_hostmap.clear();
	m_hostsmodel.clear();
	QFileInfo check_file(config_path + "/dextra.txt");
	if(check_file.exists() && check_file.isFile()){
		QFile f(config_path + "/dextra.txt");
		if(f.open(QIODevice::ReadOnly)){
			while(!f.atEnd()){
				QString l = f.readLine();
				if(l.at(0) == '#'){
					continue;
				}
				QStringList ll = l.split('\t');
				if(ll.size() > 1){
					m_hostmap[ll.at(0).simplified()] = ll.at(1).simplified() + ",30001";
				}
			}

			m_customhosts = m_localhosts.split('\n');
			for (const auto& i : m_customhosts){
				QStringList line = i.simplified().split(' ');

				if(line.at(0) == "XRF"){
					m_hostmap[line.at(1).simplified()] = line.at(2).simplified() + "," + line.at(3).simplified();
				}
			}

			QMap<QString, QString>::const_iterator i = m_hostmap.constBegin();
			while (i != m_hostmap.constEnd()) {
				m_hostsmodel.append(i.key());
				++i;
			}
		}
		f.close();
	}
	else{
		download_file("/dextra.txt");
	}
}

void DroidStar::process_ysf_hosts()
{
	m_hostmap.clear();
	m_hostsmodel.clear();
	QFileInfo check_file(config_path + "/YSFHosts.txt");
	if(check_file.exists() && check_file.isFile()){
		QFile f(config_path + "/YSFHosts.txt");
		if(f.open(QIODevice::ReadOnly)){
			while(!f.atEnd()){
				QString l = f.readLine();
				if(l.at(0) == '#'){
					continue;
				}
				QStringList ll = l.split(';');
				if(ll.size() > 4){
					m_hostmap[ll.at(1).simplified()] = ll.at(3) + "," + ll.at(4);
				}
			}

			m_customhosts = m_localhosts.split('\n');
			for (const auto& i : m_customhosts){
				QStringList line = i.simplified().split(' ');

				if(line.at(0) == "YSF"){
					m_hostmap[line.at(1).simplified()] = line.at(2).simplified() + "," + line.at(3).simplified();
				}
			}

			QMap<QString, QString>::const_iterator i = m_hostmap.constBegin();
			while (i != m_hostmap.constEnd()) {
				m_hostsmodel.append(i.key());
				++i;
			}
		}
		f.close();
		//process_fcs_rooms();
	}
	else{
		download_file("/YSFHosts.txt");
	}
}

void DroidStar::process_fcs_rooms()
{
	m_hostmap.clear();
	m_hostsmodel.clear();
	QFileInfo check_file(config_path + "/FCSHosts.txt");
	if(check_file.exists() && check_file.isFile()){
		QFile f(config_path + "/FCSHosts.txt");
		if(f.open(QIODevice::ReadOnly)){
			while(!f.atEnd()){
				QString l = f.readLine();
				if(l.at(0) == '#'){
					continue;
				}
				QStringList ll = l.split(';');
				if(ll.size() > 4){
					if(ll.at(1).simplified() != "nn"){
						m_hostmap[ll.at(0).simplified() + " - " + ll.at(1).simplified()] = ll.at(2).left(6).toLower() + ".xreflector.net,62500";
					}
				}
			}

			m_customhosts = m_localhosts.split('\n');
			for (const auto& i : m_customhosts){
				QStringList line = i.simplified().split(' ');

				if(line.at(0) == "FCS"){
					m_hostmap[line.at(1).simplified()] = line.at(2).simplified() + "," + line.at(3).simplified();
				}
			}

			QMap<QString, QString>::const_iterator i = m_hostmap.constBegin();
			while (i != m_hostmap.constEnd()) {
				m_hostsmodel.append(i.key());
				++i;
			}
		}
		f.close();
	}
	else{
		download_file("/FCSHosts.txt");
	}
}

void DroidStar::process_dmr_hosts()
{
	m_hostmap.clear();
	m_hostsmodel.clear();
	QFileInfo check_file(config_path + "/DMRHosts.txt");
	if(check_file.exists() && check_file.isFile()){
		QFile f(config_path + "/DMRHosts.txt");
		if(f.open(QIODevice::ReadOnly)){
			while(!f.atEnd()){
				QString l = f.readLine();
				if(l.at(0) == '#'){
					continue;
				}
				QStringList ll = l.simplified().split(' ');
				if(ll.size() > 4){
					if( (ll.at(0).simplified() != "DMRGateway")
					 && (ll.at(0).simplified() != "DMR2YSF")
					 && (ll.at(0).simplified() != "DMR2NXDN"))
					{
						m_hostmap[ll.at(0).simplified()] = ll.at(2) + "," + ll.at(4) + "," + ll.at(3);
					}
				}
			}

			m_customhosts = m_localhosts.split('\n');
			for (const auto& i : m_customhosts){
				QStringList line = i.simplified().split(' ');

				if(line.at(0) == "DMR"){
					m_hostmap[line.at(1).simplified()] = line.at(2).simplified() + "," + line.at(3).simplified() + "," + line.at(4).simplified();
				}
			}

			QMap<QString, QString>::const_iterator i = m_hostmap.constBegin();
			while (i != m_hostmap.constEnd()) {
				m_hostsmodel.append(i.key());
				++i;
			}
		}
		f.close();
	}
	else{
		download_file("/DMRHosts.txt");
	}
}

void DroidStar::process_p25_hosts()
{
	m_hostmap.clear();
	m_hostsmodel.clear();
	QFileInfo check_file(config_path + "/P25Hosts.txt");
	if(check_file.exists() && check_file.isFile()){
		QFile f(config_path + "/P25Hosts.txt");
		if(f.open(QIODevice::ReadOnly)){
			while(!f.atEnd()){
				QString l = f.readLine();
				if(l.at(0) == '#'){
					continue;
				}
				QStringList ll = l.simplified().split(' ');
				if(ll.size() > 2){
					m_hostmap[ll.at(0).simplified()] = ll.at(1) + "," + ll.at(2);
				}
			}

			m_customhosts = m_localhosts.split('\n');
			for (const auto& i : m_customhosts){
				QStringList line = i.simplified().split(' ');

				if(line.at(0) == "P25"){
					m_hostmap[line.at(1).simplified()] = line.at(2).simplified() + "," + line.at(3).simplified();
				}
			}

			QMap<QString, QString>::const_iterator i = m_hostmap.constBegin();
			while (i != m_hostmap.constEnd()) {
				m_hostsmodel.append(i.key());
				++i;
			}
		}
		f.close();
	}
	else{
		download_file("/P25Hosts.txt");
	}
}

void DroidStar::process_nxdn_hosts()
{
	m_hostmap.clear();
	m_hostsmodel.clear();
	QFileInfo check_file(config_path + "/NXDNHosts.txt");
	if(check_file.exists() && check_file.isFile()){
		QFile f(config_path + "/NXDNHosts.txt");
		if(f.open(QIODevice::ReadOnly)){
			while(!f.atEnd()){
				QString l = f.readLine();
				if(l.at(0) == '#'){
					continue;
				}
				QStringList ll = l.simplified().split(' ');
				if(ll.size() > 2){
					m_hostmap[ll.at(0).simplified()] = ll.at(1) + "," + ll.at(2);
				}
			}

			m_customhosts = m_localhosts.split('\n');
			for (const auto& i : m_customhosts){
				QStringList line = i.simplified().split(' ');

				if(line.at(0) == "NXDN"){
					m_hostmap[line.at(1).simplified()] = line.at(2).simplified() + "," + line.at(3).simplified();
				}
			}

			QMap<QString, QString>::const_iterator i = m_hostmap.constBegin();
			while (i != m_hostmap.constEnd()) {
				m_hostsmodel.append(i.key());
				++i;
			}
		}
		f.close();
	}
	else{
		download_file("/NXDNHosts.txt");
	}
}

void DroidStar::process_m17_hosts()
{
	m_hostmap.clear();
	m_hostsmodel.clear();
	m_hostmap["MMDVM_DIRECT"] = "MMDVM_DIRECT";

	QFileInfo check_file(config_path + "/M17Hosts-full.csv");
	if(check_file.exists() && check_file.isFile()){
		QFile f(config_path + "/M17Hosts-full.csv");
		if(f.open(QIODevice::ReadOnly)){
			while(!f.atEnd()){
				QString l = f.readLine();
				if(l.at(0) == '#'){
					continue;
				}
				QStringList ll = l.simplified().split(',');
				if(ll.size() > 3){
					m_hostmap[ll.at(0).simplified()] = ll.at(2) + "," + ll.at(4) + "," + ll.at(3);
				}
			}

			m_customhosts = m_localhosts.split('\n');
			for (const auto& i : m_customhosts){
				QStringList line = i.simplified().split(' ');

				if(line.at(0) == "M17"){
					m_hostmap[line.at(1).simplified()] = line.at(2).simplified() + "," + line.at(3).simplified();
				}
			}

			QMap<QString, QString>::const_iterator i = m_hostmap.constBegin();
			while (i != m_hostmap.constEnd()) {
				m_hostsmodel.append(i.key());
				++i;
			}
		}
		f.close();
	}
	else{
		download_file("/M17Hosts-full.csv");
	}
}

void DroidStar::process_dmr_ids()
{
	QFileInfo check_file(config_path + "/DMRIDs.dat");
	if(check_file.exists() && check_file.isFile()){
		QFile f(config_path + "/DMRIDs.dat");
		if(f.open(QIODevice::ReadOnly)){
			while(!f.atEnd()){
				QString lids = f.readLine();
				if(lids.at(0) == '#'){
					continue;
				}
				QStringList llids = lids.simplified().split(' ');

				if(llids.size() >= 2){
					m_dmrids[llids.at(0).toUInt()] = llids.at(1);
				}
			}
		}
		f.close();
	}
	else{
		download_file("/DMRIDs.dat");
	}
}

void DroidStar::update_dmr_ids()
{
	QFileInfo check_file(config_path + "/DMRIDs.dat");
	if(check_file.exists() && check_file.isFile()){
		QFile f(config_path + "/DMRIDs.dat");
		f.remove();
	}
	process_dmr_ids();
	update_nxdn_ids();
}

void DroidStar::process_nxdn_ids()
{
	QFileInfo check_file(config_path + "/NXDN.csv");
	if(check_file.exists() && check_file.isFile()){
		QFile f(config_path + "/NXDN.csv");
		if(f.open(QIODevice::ReadOnly)){
			while(!f.atEnd()){
				QString lids = f.readLine();
				if(lids.at(0) == '#'){
					continue;
				}
				QStringList llids = lids.simplified().split(',');

				if(llids.size() > 1){
					m_nxdnids[llids.at(0).toUInt()] = llids.at(1);
				}
			}
		}
		f.close();
	}
	else{
		download_file("/NXDN.csv");
	}
}

void DroidStar::update_nxdn_ids()
{
	QFileInfo check_file(config_path + "/NXDN.csv");
	if(check_file.exists() && check_file.isFile()){
		QFile f(config_path + "/NXDN.csv");
		f.remove();
	}
	process_nxdn_ids();
}

void DroidStar::update_host_files()
{
	m_update_host_files = true;
	check_host_files();
}

void DroidStar::check_host_files()
{
	if(!QDir(config_path).exists()){
		QDir().mkdir(config_path);
	}

	QFileInfo check_file(config_path + "/dplus.txt");
	if( (!check_file.exists() && !(check_file.isFile())) || m_update_host_files ){
		download_file("/dplus.txt");
	}

	check_file.setFile(config_path + "/dextra.txt");
	if( (!check_file.exists() && !check_file.isFile() ) || m_update_host_files  ){
		download_file("/dextra.txt");
	}

	check_file.setFile(config_path + "/dcs.txt");
	if( (!check_file.exists() && !check_file.isFile()) || m_update_host_files ){
		download_file( "/dcs.txt");
	}

	check_file.setFile(config_path + "/YSFHosts.txt");
	if( (!check_file.exists() && !check_file.isFile()) || m_update_host_files ){
		download_file("/YSFHosts.txt");
	}

	check_file.setFile(config_path + "/FCSHosts.txt");
	if( (!check_file.exists() && !check_file.isFile()) || m_update_host_files ){
		download_file("/FCSHosts.txt");
	}

	check_file.setFile(config_path + "/DMRHosts.txt");
	if( (!check_file.exists() && !check_file.isFile()) || m_update_host_files ){
		download_file("/DMRHosts.txt");
	}

	check_file.setFile(config_path + "/P25Hosts.txt");
	if( (!check_file.exists() && !check_file.isFile()) || m_update_host_files ){
		download_file("/P25Hosts.txt");
	}

	check_file.setFile(config_path + "/NXDNHosts.txt");
	if((!check_file.exists() && !check_file.isFile()) || m_update_host_files ){
		download_file("/NXDNHosts.txt");
	}

	check_file.setFile(config_path + "/M17Hosts-full.csv");
	if( (!check_file.exists() && !check_file.isFile()) || m_update_host_files ){
		download_file("/M17Hosts-full.csv");
	}

	check_file.setFile(config_path + "/DMRIDs.dat");
	if(!check_file.exists() && !check_file.isFile()){
		download_file("/DMRIDs.dat");
	}
	else {
		process_dmr_ids();
	}

	check_file.setFile(config_path + "/NXDN.csv");
	if(!check_file.exists() && !check_file.isFile()){
		download_file("/NXDN.csv");
	}
	else{
		process_nxdn_ids();
	}
	m_update_host_files = false;
	//process_mode_change(ui->modeCombo->currentText().simplified());
/*
#if defined(Q_OS_ANDROID)
	QString vocname = "/vocoder_plugin." + QSysInfo::productType() + "." + QSysInfo::currentCpuArchitecture();
#else
	QString vocname = "/vocoder_plugin." + QSysInfo::kernelType() + "." + QSysInfo::currentCpuArchitecture();
#endif
	QString newvoc = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation) + vocname;
	QString voc = config_path + vocname;
	check_file.setFile(newvoc);
	qDebug() << "newvoc == " << newvoc;
	qDebug() << "voc == " << voc;
	if(check_file.exists() && check_file.isFile()){
		qDebug() << newvoc << " found";
		if(QFile::exists(voc)){
			qDebug() << voc << " found";
			if(QFile::remove(voc)){
				qDebug() << voc << " deleted";
			}
			else{
				qDebug() << voc << " not deleted";
			}
		}
		if(QFile::copy(newvoc, voc)){
			qDebug() << newvoc << " copied";
		}
		else{
			qDebug() << "Could not copy " << newvoc;
		}
	}
	else{
		qDebug() << newvoc << " not found";
	}
*/
}

void DroidStar::update_data(Mode::MODEINFO info)
{
	if((connect_status == Mode::CONNECTING) && (info.status == Mode::DISCONNECTED)){
		process_connect();
		return;
	}

	if( (connect_status == Mode::CONNECTING) && ( info.status == Mode::CONNECTED_RW)){
		connect_status = Mode::CONNECTED_RW;
		emit connect_status_changed(2);
		emit in_audio_vol_changed(0.5);
		emit swtx_state(!m_mode->get_hwtx());
		emit swrx_state(!m_mode->get_hwrx());
		emit rptr2_changed(m_refname + " " + m_module);
		if(m_mycall.isEmpty()) set_mycall(m_callsign);
		if(m_urcall.isEmpty()) set_urcall("CQCQCQ");
		if(m_rptr1.isEmpty()) set_rptr1(m_callsign + " " + m_module);
		emit update_log("Connected to " + m_protocol + " " + m_refname + " " + m_host + ":" + QString::number(m_port));
	}

	m_netstatustxt = "Connected ping cnt: " + QString::number(info.count);
	m_ambestatustxt = "AMBE: " + (info.ambeprodid.isEmpty() ? "No device" : info.ambeprodid);
	m_mmdvmstatustxt = "MMDVM: ";

	if(info.mmdvm.isEmpty()){
		m_mmdvmstatustxt += "No device";
	}

	QStringList verlist = info.ambeverstr.split('.');
	if(verlist.size() > 7){
		m_ambestatustxt += " " + verlist.at(0) + " " + verlist.at(5) + " " + verlist.at(6);
	}

	verlist = info.mmdvm.split(' ');
	if(verlist.size() > 3){
		m_mmdvmstatustxt += verlist.at(0) + " " + verlist.at(1);
	}

	if(info.stream_state == Mode::STREAM_IDLE){
		m_data1.clear();
		m_data2.clear();
		m_data3.clear();
		m_data4.clear();
		m_data5.clear();
		m_data6.clear();
	}
	else if (m_protocol == "REF" || m_protocol == "XRF" || m_protocol == "DCS"){
		m_data1 = info.src;
		m_data2 = info.dst;
		m_data3 = info.gw;
		m_data4 = info.gw2;
		m_data5 = QString::number(info.streamid, 16) + " " + QString("%1").arg(info.frame_number, 2, 16, QChar('0'));
		m_data6 = info.usertxt;
	}
	else if (m_protocol == "YSF" || m_protocol == "FCS"){
		m_data1 = info.gw;
		m_data2 =info.src;
		m_data3 = info.dst;

		if(info.type == 0){
			m_data4 = "V/D mode 1";
		}
		else if(info.type == 1){
			m_data4 = "Data Full Rate";
		}
		else if(info.type == 2){
			m_data4 = "V/D mode 2";
		}
		else if(info.type == 3){
			m_data4 = "Voice Full Rate";
		}
		else{
			m_data4 = "";
		}
		if(info.type >= 0){
			m_data5 = info.path  ? "Internet" : "Local";
			m_data6 = QString::number(info.frame_number) + "/" + QString::number(info.frame_total);
		}
		else{
			m_data5 = m_data6 = "";
		}
	}
	else if(m_protocol == "DMR"){
		m_data1 = m_dmrids[info.srcid];
		m_data2 = info.srcid ? QString::number(info.srcid) : "";
		m_data3 = info.dstid ? QString::number(info.dstid) : "";
		m_data4 = info.gwid ? QString::number(info.gwid) : "";
		QString s = "Slot" + QString::number(info.slot);
		QString flco;

		switch( (info.slot & 0x40) >> 6){
		case 0:
			flco = "Group";
			break;
		case 3:
			flco = "Private";
			break;
		case 8:
			flco = "GPS";
			break;
		default:
			flco = "Unknown";
			break;
		}

		if(info.frame_number){
			QString n = s + " " + flco + " " + QString("%1").arg(info.frame_number, 2, 16, QChar('0'));
			m_data5 = n;
		}
	}
	else if(m_protocol == "P25"){
		m_data1 = m_dmrids[info.srcid];
		m_data2 = info.srcid ? QString::number(info.srcid) : "";
		m_data3 = info.dstid ? QString::number(info.dstid) : "";
		m_data4 = info.srcid ? QString::number(info.srcid) : "";
		if(info.frame_number){
			QString n = QString("%1").arg(info.frame_number, 2, 16, QChar('0'));
			m_data5 = n;
		}
	}
	else if(m_protocol == "NXDN"){
		if(info.srcid){
			m_data1 = m_nxdnids[info.srcid];
			m_data2 = QString::number(info.srcid);
		}
		m_data3 = QString::number(info.dstid);

		if(info.frame_number){
			QString n = QString("%1").arg(info.frame_number, 4, 16, QChar('0'));
			m_data5 = n;
		}
	}
	else if(m_protocol == "M17"){
		m_data1 = info.src;
		m_data2 = info.dst;
		m_data3 = info.type ? "3200 Voice" : "1600 V/D";
		if(info.frame_number){
			QString n = QString("%1").arg(info.frame_number, 4, 16, QChar('0'));
			m_data4 = n;
		}
		m_data5 = QString::number(info.streamid, 16);
	}
	else if(m_protocol == "IAX"){

	}
	QString t = QDateTime::fromMSecsSinceEpoch(info.ts).toString("yyyy.MM.dd hh:mm:ss.zzz");
	if(info.stream_state == Mode::STREAM_NEW){
		emit update_log(t + " " + m_protocol + " RX started id: " + QString::number(info.streamid, 16) + " src: " + info.src + " dst: " + info.gw2);
	}
	if(info.stream_state == Mode::STREAM_END){
		emit update_log(t + " " + m_protocol + " RX ended id: " + QString::number(info.streamid, 16) + " src: " + info.src + " dst: " + info.gw2);
	}
	if(info.stream_state == Mode::STREAM_LOST){
		emit update_log(t + " " + m_protocol + " RX lost id: " + QString::number(info.streamid, 16) + " src: " + info.src + " dst: " + info.gw2);
	}
	emit update_data();
}

void DroidStar::set_input_volume(qreal v)
{
	emit in_audio_vol_changed(v);
	//audioin->setVolume(v * 0.01);
}

void DroidStar::press_tx()
{
	emit tx_pressed();
}

void DroidStar::release_tx()
{
	emit tx_released();
}

void DroidStar::click_tx(bool tx)
{
	emit tx_clicked(tx);
}
