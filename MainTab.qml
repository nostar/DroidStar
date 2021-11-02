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

import QtQuick 2.10
//import QtQuick.Window 2.15
import QtQuick.Controls 2.3
//import QtQuick.Dialogs 1.3
//import org.dudetronics.droidstar 1.0

Item {
	id: mainTab
	property alias element1: _element1
	property alias element3: _element3
	property alias element4: _element4
	property alias label1: _label1
	property alias label2: _label2
	property alias label3: _label3
	property alias label4: _label4
	property alias label5: _label5
	property alias label6: _label6
	property alias status: _status
	property alias levelMeter: _levelMeter
	property alias uitimer: _uitimer
	property alias comboMode: _comboMode
	property alias comboHost: _comboHost
	property alias editIAXDTMF: _editIAXDTMF
	property alias dtmfsendbutton: _dtmfsendbutton
	property alias comboModule: _comboModule
	property alias dmrtgidEdit: _dmrtgidEdit
	property alias privateBox: _privateBox
	property alias connectbutton: _connectbutton
	property alias data1: _data1
	property alias data2: _data2
	property alias data3: _data3
	property alias data4: _data4
	property alias data5: _data5
	property alias data6: _data6
	property alias txtimer: _txtimer
	property alias buttonTX: _buttonTX
	property alias btntxt: _btntxt
	property alias swtxBox: _swtxBox
	property alias swrxBox: _swrxBox
	property alias agcBox: _agcBox

	Text {
		id: element
		x: 10
		y: 0
		width: parent.width / 4
		height: parent.height / 15;
		text: qsTr("Mode")
		color: "white"
		font.pixelSize: parent.height / 30;
		verticalAlignment: Text.AlignVCenter
	}

	Text {
		id: _element1
		x: 10
		y: (parent.height / 15 + 3) * 1;
		width: parent.width / 4
		height:  parent.height / 15;
		text: qsTr("Host")
		color: "white"
		font.pixelSize: parent.height / 30;
		verticalAlignment: Text.AlignVCenter
	}

	Text {
		id: _element4
		x: 10
		y: (parent.height / 15 + 3) * 2;
		width: parent.width / 5
		height:  parent.height / 15;
		text: qsTr("Mod")
		color: "white"
		font.pixelSize: parent.height / 30;
		verticalAlignment: Text.AlignVCenter
	}

	Text {
		id: _element3
		x: 10
		y: (parent.height / 15 + 3) * 3;
		width: parent.width / 4
		height:  parent.height / 15;
		text: qsTr("TG ID")
		color: "white"
		font.pixelSize: parent.height / 30;
		verticalAlignment: Text.AlignVCenter
	}

	Text {
		id: _label1
		x: 10
		y: (parent.height / 15 + 3) * 4;
		width: parent.width / 3
		height: parent.height / 20;
		text: qsTr("MYCALL")
		color: "white"
		font.pixelSize: parent.height / 30;
	}

	Text {
		id: _label2
		x: 10
		y: (parent.height / 15 + 3) * 5;
		width: parent.width / 3
		height: parent.height / 20;
		text: qsTr("URCALL")
		color: "white"
		font.pixelSize: parent.height / 30;
	}

	Text {
		id: _label3
		x: 10
		y: (parent.height / 15 + 3) * 6;
		width: parent.width / 3
		height: parent.height / 20;
		text: qsTr("RPTR1")
		color: "white"
		font.pixelSize: parent.height / 30;
	}

	Text {
		id: _label4
		x: 10
		y: (parent.height / 15 + 3) * 7;
		width: parent.width / 3
		height: parent.height / 20;
		text: qsTr("RPTR2")
		color: "white"
		font.pixelSize: parent.height / 30;
	}

	Text {
		id: _label5
		x: 10
		y: (parent.height / 15 + 3) * 8;
		width: parent.width / 3
		height: parent.height / 20;
		text: qsTr("StrmID")
		color: "white"
		font.pixelSize: parent.height / 30;
	}

	Text {
		id: _label6
		x: 10
		y: (parent.height / 15 + 3) * 9;
		width: parent.width / 3
		height: parent.height / 20;
		text: qsTr("Text")
		color: "white"
		font.pixelSize: parent.height / 30;
	}

	Text {
		id: _status
		x: 10
		y: (parent.height / 15 + 3) * 10;
		width: parent.width - 20
		height: parent.height / 20;
		text: qsTr("Not Connected")
		color: "white"
		font.pixelSize: parent.height / 35;
	}
	Rectangle {
		x: 10
		y: (parent.height / 15 + 3) * 11;
		width: parent.width - 20
		height: parent.height / 20;
		color: "black"
		border.color: "black"
		border.width: 2
		radius: 5
	}
	Rectangle {
		id: _levelMeter
		x: 10
		y: (parent.height / 15 + 3) * 11;
		width: 0
		height: parent.height / 20;
		color: "#80C342"
		border.color: "black"
		border.width: 2
		radius: 5
	}
	Timer {
		id: _uitimer
		interval: 20; running: true; repeat: true
		property var cnt: 0;
		property var rxcnt: 0;
		property var last_rxcnt: 0;
		onTriggered: update_level();

		function update_level(){
			if(cnt >= 20){
				if(rxcnt == last_rxcnt){
					droidstar.set_output_level(0);
					rxcnt = 0;
					//console.log("TIMEOUT");
				}
				else{
					last_rxcnt = rxcnt;
				}

				cnt = 0;
			}
			else{
				++cnt;
			}

			var l = (parent.width - 20) * droidstar.get_output_level() / 32767.0;
			if(l > _levelMeter.width){
				_levelMeter.width = l;
			}
			else{
				if(_levelMeter.width > 0)
					_levelMeter.width -= 8;
				else
					_levelMeter.width = 0;
			}
		}
	}
	ComboBox {
		id: _comboMode
		property bool loaded: false
		x: parent.width / 4
		y: 0
		width: (parent.width * 3 / 8) - 4
		height: parent.height / 15;
		font.pixelSize: parent.height / 35
		model: ["M17", "YSF", "FCS", "DMR", "P25", "NXDN", "REF", "XRF", "DCS", "IAX"]
		contentItem: Text {
			text: _comboMode.displayText
			font: _comboMode.font
			leftPadding: 10
			verticalAlignment: Text.AlignVCenter
			color: _comboMode.enabled ? "white" : "darkgrey"
		}
		onCurrentTextChanged: {
			if(_comboMode.loaded){
				droidstar.process_mode_change(_comboMode.currentText);
			}
		}
	}

	ComboBox {
		id: _comboHost
		x: (parent.width / 4)
		y: (parent.height / 15 + 3) * 1;
		width: ((parent.width * 3) / 4) - 5
		height: parent.height / 15;
		font.pixelSize: parent.height / 35
		contentItem: Text {
			text: _comboHost.displayText
			font: _comboHost.font
			leftPadding: 10
			verticalAlignment: Text.AlignVCenter
			color: _comboHost.enabled ? "white" : "darkgrey"
		}
		onCurrentTextChanged: {
			if(!droidstar.get_modelchange()){
				droidstar.process_host_change(_comboHost.currentText);
			}
		}
	}
	TextField {
		id: _editIAXDTMF
		x: (parent.width / 4)
		y: (parent.height / 15 + 3) * 1;
		width: (parent.width * 3 / 8) - 4;
		height: parent.height / 15;
		font.pixelSize: parent.height / 35
		inputMethodHints: "ImhPreferNumbers"
	}
	Button {
		id: _dtmfsendbutton
		x: (parent.width * 5 / 8)
		y: (parent.height / 15 + 3) * 1;
		width: (parent.width * 3 / 8) - 5;
		height: parent.height / 15;
		text: qsTr("Send")
		font.pixelSize: parent.height / 30
		onClicked: {
			droidstar.dtmf_send_clicked(editIAXDTMF.text);
		}
	}

	ComboBox {
		id: _comboModule
		x: parent.width / 5
		y: (parent.height / 15 + 3) * 2
		width: parent.width / 5
		height: parent.height / 15;
		font.pixelSize: parent.height / 35
		model: ["A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z"]
		contentItem: Text {
			text: _comboModule.displayText
			font: _comboModule.font
			leftPadding: 10
			verticalAlignment: Text.AlignVCenter
			color: _comboModule.enabled ? "white" : "darkgrey"
		}
		onCurrentTextChanged: {
			if(_comboMode.loaded){
				droidstar.set_module(_comboModule.currentText);
			}
		}
	}

	CheckBox {
		id: _swtxBox
		x: (parent.width * 2 / 5)
		y: (parent.height / 15 + 3) * 2;
		width: parent.width / 4
		height: parent.height / 15
		text: qsTr("SWTX")
		onClicked:{
			droidstar.set_swtx(_swtxBox.checked)
		}
	}

	CheckBox {
		id: _swrxBox
		x: (parent.width * 3 / 5)
		y: (parent.height / 15 + 3) * 2;
		width: parent.width / 4
		height: parent.height / 15
		text: qsTr("SWRX")
		onClicked:{
			droidstar.set_swrx(_swrxBox.checked)
		}
	}

	CheckBox {
		id: _agcBox
		x: (parent.width * 4 / 5)
		y: (parent.height / 15 + 3) * 2;
		width: parent.width / 4
		height: parent.height / 15
		text: qsTr("AGC")
		onClicked:{
			droidstar.set_agc(_agcBox.checked)
		}
	}

	TextField {
		visible: false
		id: _dmrtgidEdit
		x: parent.width / 4
		y: (parent.height / 15 + 3) * 3
		width: (parent.width * 3 / 8) - 4
		height: parent.height / 15
		font.pixelSize: parent.height / 30
		selectByMouse: true
		inputMethodHints: "ImhPreferNumbers"
		text: qsTr("")
		onEditingFinished: {
			droidstar.tgid_text_changed(dmrtgidEdit.text)
		}
	}
	CheckBox {
		id: _privateBox
		x: (parent.width * 5 / 8)
		y: (parent.height / 15 + 3) * 3;
		width: (parent.width * 3 / 8) - 5
		height: parent.height / 15
		text: qsTr("Private")
		onClicked:{
			droidstar.set_dmr_pc(privateBox.checked)
			//console.log("screen size ", parent.width, " x ", parent.height);
		}
	}

	Button {
		id: _connectbutton
		x: (parent.width * 5 / 8)
		y: 0
		width: (parent.width * 3 / 8) - 5
		height: parent.height / 15
		text: qsTr("Connect")
		font.pixelSize: parent.height / 30
		onClicked: {
			//settingsTab.callsignEdit.text = settingsTab.callsignEdit.text.toUpperCase();
			droidstar.set_callsign(settingsTab.callsignEdit.text.toUpperCase());
			//droidstar.set_host(comboHost.currentText);
			droidstar.set_module(comboModule.currentText);
			droidstar.set_protocol(comboMode.currentText);
			droidstar.set_dmrtgid(dmrtgidEdit.text);
			droidstar.set_dmrid(settingsTab.dmridEdit.text);
			droidstar.set_essid(settingsTab.comboEssid.currentText);
			droidstar.set_bm_password(settingsTab.bmpwEdit.text);
			droidstar.set_tgif_password(settingsTab.tgifpwEdit.text);
			droidstar.set_latitude(settingsTab.latEdit.text);
			droidstar.set_longitude(settingsTab.lonEdit.text);
			droidstar.set_location(settingsTab.locEdit.text);
			droidstar.set_description(settingsTab.descEdit.text);
			droidstar.set_url(settingsTab.urlEdit.text);
			droidstar.set_swid(settingsTab.swidEdit.text);
			droidstar.set_pkgid(settingsTab.pkgidEdit.text);
			droidstar.set_dmr_options(settingsTab.dmroptsEdit.text);
			droidstar.set_iaxuser(settingsTab.iaxuserEdit.text);
			droidstar.set_iaxpass(settingsTab.iaxpassEdit.text);
			droidstar.set_iaxnode(settingsTab.iaxnodeEdit.text);
			droidstar.set_iaxhost(settingsTab.iaxhostEdit.text);
			droidstar.set_iaxport(settingsTab.iaxportEdit.text);
			droidstar.set_txtimeout(settingsTab.txtimerEdit.text);
			//droidstar.set_toggletx(toggleTX.checked);
			droidstar.set_xrf2ref(settingsTab.xrf2ref.checked);
			droidstar.set_ipv6(settingsTab.ipv6.checked);
			droidstar.set_vocoder(settingsTab.comboVocoder.currentText);
			droidstar.set_modem(settingsTab.comboModem.currentText);
			droidstar.set_playback(settingsTab.comboPlayback.currentText);
			droidstar.set_capture(settingsTab.comboCapture.currentText);

			droidstar.set_modemRxFreq(settingsTab.modemRXFreqEdit.text);
			droidstar.set_modemTxFreq(settingsTab.modemTXFreqEdit.text);
			droidstar.set_modemRxOffset(settingsTab.modemRXOffsetEdit.text);
			droidstar.set_modemTxOffset(settingsTab.modemTXOffsetEdit.text);
			droidstar.set_modemRxDCOffset(settingsTab.modemRXDCOffsetEdit.text);
			droidstar.set_modemTxDCOffset(settingsTab.modemTXDCOffsetEdit.text);
			droidstar.set_modemRxLevel(settingsTab.modemRXLevelEdit.text);
			droidstar.set_modemTxLevel(settingsTab.modemRXLevelEdit.text);
			droidstar.set_modemRFLevel(settingsTab.modemRFLevelEdit.text);
			droidstar.set_modemTxDelay(settingsTab.modemTXDelayEdit.text);
			droidstar.set_modemCWIdTxLevel(settingsTab.modemCWIdTXLevelEdit.text);
			droidstar.set_modemDstarTxLevel(settingsTab.modemDStarTXLevelEdit.text);
			droidstar.set_modemDMRTxLevel(settingsTab.modemDMRTXLevelEdit.text);
			droidstar.set_modemYSFTxLevel(settingsTab.modemYSFTXLevelEdit.text);
			droidstar.set_modemP25TxLevel(settingsTab.modemYSFTXLevelEdit.text);
			droidstar.set_modemNXDNTxLevel(settingsTab.modemNXDNTXLevelEdit.text);

			droidstar.process_connect();
		}
	}

	Text {
		id: _data1
		x: parent.width / 3
		y: (parent.height / 15 + 3) * 4;
		width: (parent.width * 2) / 3
		height: parent.height / 20;
		text: qsTr("")
		color: "white"
		font.pixelSize: parent.height / 30;
	}

	Text {
		id: _data2
		x: parent.width / 3
		y: (parent.height / 15 + 3) * 5;
		width: (parent.width * 2) / 3
		height: parent.height / 20;
		text: qsTr("")
		color: "white"
		font.pixelSize: parent.height / 30;
	}

	Text {
		id: _data3
		x: parent.width / 3
		y: (parent.height / 15 + 3) * 6;
		width: (parent.width * 2) / 3
		height: parent.height / 20;
		text: qsTr("")
		color: "white"
		font.pixelSize: parent.height / 30;
	}

	Text {
		id: _data4
		x: parent.width / 3
		y: (parent.height / 15 + 3) * 7;
		width: (parent.width * 2) / 3
		height: parent.height / 20;
		text: qsTr("")
		color: "white"
		font.pixelSize: parent.height / 30;
	}

	Text {
		id: _data5
		x: parent.width / 3
		y: (parent.height / 15 + 3) * 8;
		width: (parent.width * 2) / 3
		height: parent.height / 20;
		text: qsTr("")
		color: "white"
		font.pixelSize: parent.height / 30;
	}

	Text {
		id: _data6
		x: parent.width / 3
		y:(parent.height / 15 + 3) * 9;
		width: (parent.width * 2) / 3
		height: parent.height / 20;
		text: qsTr("")
		color: "white"
		font.pixelSize: parent.height / 30;
	}

	Button {
		Timer {
			id: _txtimer
			repeat: true;
			onTriggered: {
				++buttonTX.cnt;
				btntxt.text = "TX: " + buttonTX.cnt;
				if(buttonTX.cnt >= parseInt(settingsTab.txtimerEdit.text)){
					buttonTX.tx = false;
					droidstar.click_tx(buttonTX.tx);
					_txtimer.running = false;
					_btntxt.text = "TX";
				}
			}
		}

		property bool tx: false
		property int cnt: 0
		visible: true
		enabled: false
		id: _buttonTX
		background: Rectangle {
			color: _buttonTX.tx ? "#800000" : "steelblue" //"#80c342"
			radius: 10
			Text {
				id:_btntxt
				anchors.centerIn: parent
				font.pointSize: 18
				text: qsTr("TX")
			}
		}
		x: 10
		y: (parent.height / 15 + 3) * 12;
		//y: parent.height - ((parent.height / 5) + 5);
		width: parent.width - 20
		height: parent.height - y - 10
		//text: qsTr("TX")
		font.pointSize: 24
		onClicked: {
			if(settingsTab.toggleTX.checked){
				tx = !tx;
				droidstar.click_tx(tx);
				if(tx){
					cnt = 0;
					_txtimer.running = true;
					_btntxt.color = "white";
				}
				else{
					_txtimer.running = false;
					btntxt.color = "black";
					_btntxt.text = "TX";
				}
			}
		}
		onPressed: {
			if(!settingsTab.toggleTX.checked){
				tx = true;
				droidstar.press_tx();
			}
		}
		onReleased: {
			if(!settingsTab.toggleTX.checked){
				tx = false;
				droidstar.release_tx();
			}
		}
		onCanceled: {
			if(!settingsTab.toggleTX.checked){
				tx = false;
				droidstar.release_tx();
			}
		}
	}
}
