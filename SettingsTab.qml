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
import QtQuick.Controls 2.3

Item {
	id: settingsTab
	property alias callsignEdit: csedit
	property alias dmridEdit: dmridedit
	property alias comboEssid: comboessid
	property alias bmpwEdit: bmpwedit
	property alias tgifpwEdit: tgifpwedit
	property alias latEdit: latedit
	property alias lonEdit: lonedit
	property alias locEdit: locedit
	property alias descEdit: descedit
	property alias urlEdit: urledit
	property alias swidEdit: swidedit
	property alias pkgidEdit: pkgidedit
	property alias dmroptsEdit: dmroptsedit
	property alias iaxuserEdit: iaxuseredit
	property alias iaxpassEdit: iaxpassedit
	property alias iaxnodeEdit: iaxnodeedit
	property alias iaxhostEdit: iaxhostedit
	property alias iaxportEdit: iaxportedit
	property alias m173200: m17_3200
	property alias m171600: m17_1600
	property alias sliderMicGain: slidermicGain
	property alias mycallEdit: mycalledit
	property alias urcallEdit: urcalledit
	property alias rptr1Edit: rptr1edit
	property alias rptr2Edit: rptr2edit
	property alias txtimerEdit: txtimeredit
	property alias toggleTX: toggletx
	property alias xrf2ref: xrf2Ref
	property alias ipv6: ipV6
	property alias comboVocoder: _comboVocoder
	property alias comboModem: _comboModem
	property alias comboPlayback: _comboPlayback
	property alias comboCapture: _comboCapture
	property alias modemRXFreqEdit: _modemRXFreqEdit
	property alias modemTXFreqEdit: _modemTXFreqEdit
	property alias modemRXOffsetEdit: _modemRXOffsetEdit
	property alias modemTXOffsetEdit: _modemTXOffsetEdit
	property alias modemRXDCOffsetEdit: _modemRXDCOffsetEdit
	property alias modemTXDCOffsetEdit: _modemTXDCOffsetEdit
	property alias modemRXLevelEdit: _modemRXLevelEdit
	property alias modemTXLevelEdit: _modemTXLevelEdit
	property alias modemRFLevelEdit: _modemRFLevelEdit
	property alias modemTXDelayEdit: _modemTXDelayEdit
	property alias modemCWIdTXLevelEdit: _modemCWIdTXLevelEdit
	property alias modemDStarTXLevelEdit: _modemDStarTXLevelEdit
	property alias modemDMRTXLevelEdit: _modemDMRTXLevelEdit
	property alias modemYSFTXLevelEdit: _modemYSFTXLevelEdit
	property alias modemP25TXLevelEdit: _modemP25TXLevelEdit
	property alias modemNXDNTXLevelEdit: _modemNXDNTXLevelEdit

	Flickable {
		anchors.fill: parent
		contentWidth: parent.width
		contentHeight: vocoderButton.y +
					   vocoderButton.height + 10
		flickableDirection: Flickable.VerticalFlick
		clip: true

		Text {
			id: csLabel
			x: 10
			y: 0
			width: 80
			height: 25
			text: qsTr("Callsign")
			color: "white"
			verticalAlignment: Text.AlignVCenter
		}
		TextField {
			id: csedit
			x: 100
			y: 0
			width: 125
			height: 25
			text: qsTr("")
			font.capitalization: Font.AllUppercase
			selectByMouse: true
		}
		Text {
			id: dmridLabel
			x: 10
			y: 30
			width: 80
			height: 25
			text: qsTr("DMRID")
			color: "white"
			verticalAlignment: Text.AlignVCenter
		}
		TextField {
			id: dmridedit
			x: 100
			y: 30
			width: 125
			height: 25
			selectByMouse: true
			inputMethodHints: "ImhPreferNumbers"
		}
		Text {
			id: essidLabel
			x: 10
			y: 60
			width: 80
			height: 25
			text: qsTr("ESSID")
			color: "white"
			verticalAlignment: Text.AlignVCenter
		}
		ComboBox {
			id: comboessid
			x: 100
			y: 60
			width: 60
			height: 30
			function build_model(){
				console.log("build_model() called");
				var ids = ["None"];
				for(var i = 0; i < 100; ++i){
					ids[i+1] = i.toString().padStart(2, "0");
				}
					comboessid.model = ids;
					comboessid.currentIndex = comboessid.find(droidstar.get_essid());
			}

			Component.onCompleted: build_model();
			onCurrentTextChanged: {
				//console.log("set essid called");
				//droidstar.set_essid(comboessid.currentText);
			}
		}
		Text {
			id: bmpwLabel
			x: 10
			y: 95
			width: 80
			height: 25
			text: qsTr("BM Pass")
			color: "white"
			verticalAlignment: Text.AlignVCenter
		}
		TextField {
			id: bmpwedit
			x: 100
			y: 95
			width: parent.width - 110
			height: 25
			selectByMouse: true
			echoMode: TextInput.Password
		}
		Text {
			id: tgifpwLabel
			x: 10
			y: 130
			width: 80
			height: 25
			text: qsTr("TGIF Pass")
			color: "white"
			verticalAlignment: Text.AlignVCenter
		}
		TextField {
			id: tgifpwedit
			x: 100
			y: 130
			width: parent.width - 110
			height: 25
			selectByMouse: true
			echoMode: TextInput.Password
		}
		Text {
			id: latLabel
			x: 10
			y: 160
			width: 80
			height: 25
			text: qsTr("Latitude")
			color: "white"
			verticalAlignment: Text.AlignVCenter
		}
		TextField {
			id: latedit
			x: 100
			y: 160
			width: 125
			height: 25
			selectByMouse: true
		}
		Text {
			id: lonLabel
			x: 10
			y: 190
			width: 80
			height: 25
			text: qsTr("Longitude")
			color: "white"
			verticalAlignment: Text.AlignVCenter
		}
		TextField {
			id: lonedit
			x: 100
			y: 190
			width: 125
			height: 25
			selectByMouse: true
		}
		Text {
			id: locLabel
			x: 10
			y: 220
			width: 80
			height: 25
			text: qsTr("Location")
			color: "white"
			verticalAlignment: Text.AlignVCenter
		}
		TextField {
			id: locedit
			x: 100
			y: 220
			width: 125
			height: 25
			selectByMouse: true
		}
		Text {
			id: descLabel
			x: 10
			y: 250
			width: 80
			height: 25
			text: qsTr("Description")
			color: "white"
			verticalAlignment: Text.AlignVCenter
		}
		TextField {
			id: descedit
			x: 100
			y: 250
			width: 125
			height: 25
			selectByMouse: true
		}
		Text {
			id: urlLabel
			x: 10
			y: 280
			width: 80
			height: 25
			text: qsTr("URL")
			color: "white"
			verticalAlignment: Text.AlignVCenter
		}
		TextField {
			id: urledit
			x: 100
			y: 280
			width: 125
			height: 25
			selectByMouse: true
		}
		Text {
			id: swidLabel
			x: 10
			y: 310
			width: 80
			height: 25
			text: qsTr("SoftwareID")
			color: "white"
			verticalAlignment: Text.AlignVCenter
		}
		TextField {
			id: swidedit
			x: 100
			y: 310
			width: 125
			height: 25
			selectByMouse: true
		}
		Text {
			id: pkgidLabel
			x: 10
			y: 340
			width: 80
			height: 25
			text: qsTr("PackageID")
			color: "white"
			verticalAlignment: Text.AlignVCenter
		}
		TextField {
			id: pkgidedit
			x: 100
			y: 340
			width: 125
			height: 25
			selectByMouse: true
		}
		Text {
			id: dmroptslabel
			x: 10
			y: 370
			width: 80
			height: 25
			text: qsTr("DMR+ Opts")
			color: "white"
			verticalAlignment: Text.AlignVCenter
		}
		TextField {
			id: dmroptsedit
			x: 100
			y: 370
			width: parent.width - 110
			height: 25
			selectByMouse: true
		}
		Text {
			id: iaxuserlabel
			x: 10
			y: 400
			width: 80
			height: 25
			text: qsTr("IAX User")
			color: "white"
			verticalAlignment: Text.AlignVCenter
		}
		TextField {
			id: iaxuseredit
			x: 100
			y: 400
			width: 125
			height: 25
			selectByMouse: true
		}
		Text {
			id: iaxpasslabel
			x: 10
			y: 430
			width: 80
			height: 25
			text: qsTr("IAX secret")
			color: "white"
			verticalAlignment: Text.AlignVCenter
		}
		TextField {
			id: iaxpassedit
			x: 100
			y: 430
			width: 125
			height: 25
			selectByMouse: true
			echoMode: TextInput.Password
		}
		Text {
			id: iaxnodelabel
			x: 10
			y: 460
			width: 80
			height: 25
			text: qsTr("IAX Node")
			color: "white"
			verticalAlignment: Text.AlignVCenter
		}
		TextField {
			id: iaxnodeedit
			x: 100
			y: 460
			width: 125
			height: 25
			selectByMouse: true
			inputMethodHints: "ImhPreferNumbers"
		}
		Text {
			id: iaxhostlabel
			x: 10
			y: 490
			width: 80
			height: 25
			text: qsTr("IAX Host")
			color: "white"
			verticalAlignment: Text.AlignVCenter
		}
		TextField {
			id: iaxhostedit
			x: 100
			y: 490
			width: 125
			height: 25
			selectByMouse: true
		}
		Text {
			id: iaxportlabel
			x: 10
			y: 520
			width: 80
			height: 25
			text: qsTr("IAX Port")
			color: "white"
			verticalAlignment: Text.AlignVCenter
		}
		TextField {
			id: iaxportedit
			x: 100
			y: 520
			width: 125
			height: 25
			selectByMouse: true
			inputMethodHints: "ImhPreferNumbers"
		}
		Text {
			id: m17rateLabel
			x: 10
			y: 550
			width: 100
			height: 25
			text: qsTr("M17/YSF rate")
			color: "white"
			verticalAlignment: Text.AlignVCenter
		}
		ButtonGroup {
			id: m17rateGroup
			onClicked: {
				button.text == "Voice Full" ? droidstar.m17_rate_changed(true) : droidstar.m17_rate_changed(false)
			}
		}
		CheckBox {
			id: m17_3200
			x: 120
			y: 550
			//width: 100
			height: 25
			spacing: 1
			text: qsTr("Voice Full")
			checked: true
			ButtonGroup.group: m17rateGroup
		}
		CheckBox {
			id: m17_1600
			x: 220
			y: 550
			//width: 100
			height: 25
			spacing: 1
			text: qsTr("Voice/Data")
			ButtonGroup.group: m17rateGroup
		}
		Text {
			id: micgain_label
			x: 10
			y: 590
			width: 80
			height: 25
			text: qsTr("Mic gain")
			color: "white"
			verticalAlignment: Text.AlignVCenter
		}
		Slider {
			visible: true
			id: slidermicGain
			x: 72
			y: 590
			width: parent.parent.width - 80
			height: parent.parent.height / 15
			value: 0.5
			onValueChanged: {
				droidstar.set_input_volume(value);
			}
		}
		Button {
			id: updatehostsButton
			x: 10
			y: 630
			width: 150
			height: 30
			text: qsTr("Update hosts")
			onClicked: {
				droidstar.update_host_files()
				updateDialog.open()
			}
		}
		Button {
			id: updatedmridsButton
			x: 170
			y: 630
			width: 150
			height: 30
			text: qsTr("Update ID files")
			onClicked: {
				droidstar.update_dmr_ids()
				updateDialog.open()
			}
		}
		Text {
			id: mycallLabel
			x: 10
			y: 700
			width: 80
			height: 25
			text: qsTr("MYCALL")
			color: "white"
			verticalAlignment: Text.AlignVCenter
		}
		TextField {
			id: mycalledit
			x: 100
			y: 700
			width: 125
			height: 25
			selectByMouse: true
			font.capitalization: Font.AllUppercase
			onEditingFinished: {
				droidstar.set_mycall(mycalledit.text.toUpperCase())
			}
		}
		Text {
			id: urcallLabel
			x: 10
			y: 730
			width: 80
			height: 25
			text: qsTr("URCALL")
			color: "white"
			verticalAlignment: Text.AlignVCenter
		}
		TextField {
			id: urcalledit
			x: 100
			y: 730
			width: 125
			height: 25
			selectByMouse: true
			font.capitalization: Font.AllUppercase
			onEditingFinished: {
				droidstar.set_urcall(urcalledit.text.toUpperCase())
			}
		}
		Text {
			id: rptr1Label
			x: 10
			y: 760
			width: 80
			height: 25
			text: qsTr("RPTR1")
			color: "white"
			verticalAlignment: Text.AlignVCenter
		}
		TextField {
			id: rptr1edit
			x: 100
			y: 760
			width: 125
			height: 25
			selectByMouse: true
			font.capitalization: Font.AllUppercase
			onEditingFinished: {
				droidstar.set_rptr1(rptr1edit.text.toUpperCase())
			}
		}
		Text {
			id: rptr2Label
			x: 10
			y: 790
			width: 80
			height: 25
			text: qsTr("RPTR2")
			color: "white"
			verticalAlignment: Text.AlignVCenter
		}
		TextField {
			id: rptr2edit
			x: 100
			y: 790
			width: 125
			height: 25
			selectByMouse: true
			font.capitalization: Font.AllUppercase
			onEditingFinished: {
				droidstar.set_rptr2(rptr2edit.text.toUpperCase())
			}
		}
		Text {
			id: txtimerLabel
			x: 10
			y: 820
			width: 80
			height: 25
			text: qsTr("TX Timeout")
			color: "white"
			verticalAlignment: Text.AlignVCenter
		}
		TextField {
			id: txtimeredit
			x: 100
			y: 820
			width: 125
			height: 25
			selectByMouse: true
		}
		CheckBox {
			id: toggletx
			x: 10
			y: 850
			//width: 100
			height: 25
			spacing: 1
			text: qsTr("Enable TX toggle mode")
			onClicked:{
				droidstar.set_toggletx(toggleTX.checked);
			}
		}
		CheckBox {
			id: xrf2Ref
			x: 10
			y: 880
			//width: 100
			height: 25
			spacing: 1
			text: qsTr("Use REF for XRF")
		}
		CheckBox {
			id: ipV6
			x: 10
			y: 910
			//width: 100
			height: 25
			spacing: 1
			text: qsTr("Use IPv6 when available")
		}
		Text {
			id: vocoderLabel
			x: 10
			y: 940
			width: 80
			height: 25
			text: qsTr("Vocoder")
			color: "white"
			verticalAlignment: Text.AlignVCenter
		}
		ComboBox {
			id: _comboVocoder
			x: 100
			y: 940
			width: parent.width - 110
			height: 30
		}
		Text {
			id: modemLabel
			x: 10
			y: 970
			width: 80
			height: 25
			text: qsTr("Modem")
			color: "white"
			verticalAlignment: Text.AlignVCenter
		}
		ComboBox {
			id: _comboModem
			x: 100
			y: 970
			width: parent.width - 110
			height: 30
		}
		Text {
			id: playbackLabel
			x: 10
			y: 1000
			width: 80
			height: 25
			text: qsTr("Playback")
			color: "white"
			verticalAlignment: Text.AlignVCenter
		}
		ComboBox {
			id: _comboPlayback
			x: 100
			y: 1000
			width: parent.width - 110
			height: 30
		}
		Text {
			id: captureLabel
			x: 10
			y: 1030
			width: 80
			height: 25
			text: qsTr("Capture")
			color: "white"
			verticalAlignment: Text.AlignVCenter
		}
		ComboBox {
			id: _comboCapture
			x: 100
			y: 1030
			width: parent.width - 110
			height: 30
		}
		Text {
			id: _modemRXFreqLabel
			x: 10
			y: 1070
			width: 80
			height: 25
			text: qsTr("RX Freq")
			color: "white"
			verticalAlignment: Text.AlignVCenter
		}
		TextField {
			id: _modemRXFreqEdit
			x: 100
			y: 1070
			width: parent.width - 110
			height: 25
			selectByMouse: true
			inputMethodHints: "ImhPreferNumbers"
		}
		Text {
			id: _modemTXFreqLabel
			x: 10
			y: 1100
			width: 80
			height: 25
			text: qsTr("TX Freq")
			color: "white"
			verticalAlignment: Text.AlignVCenter
		}
		TextField {
			id: _modemTXFreqEdit
			x: 100
			y: 1100
			width: parent.width - 110
			height: 25
			selectByMouse: true
			inputMethodHints: "ImhPreferNumbers"
		}
		Text {
			id: _modemRXOffsetLabel
			x: 10
			y: 1130
			width: 80
			height: 25
			text: qsTr("RX Offset")
			color: "white"
			verticalAlignment: Text.AlignVCenter
		}
		TextField {
			id: _modemRXOffsetEdit
			x: 100
			y: 1130
			width: parent.width - 110
			height: 25
			selectByMouse: true
			//inputMethodHints: "ImhPreferNumbers"
		}
		Text {
			id: _modemTXOffsetLabel
			x: 10
			y: 1160
			width: 80
			height: 25
			text: qsTr("TX Offset")
			color: "white"
			verticalAlignment: Text.AlignVCenter
		}
		TextField {
			id: _modemTXOffsetEdit
			x: 100
			y: 1160
			width: parent.width - 110
			height: 25
			selectByMouse: true
			//inputMethodHints: "ImhPreferNumbers"
		}
		Text {
			id: _modemRXLevelLabel
			x: 10
			y: 1190
			width: 80
			height: 25
			text: qsTr("RX Level")
			color: "white"
			verticalAlignment: Text.AlignVCenter
		}
		TextField {
			id: _modemRXLevelEdit
			x: 100
			y: 1190
			width: parent.width - 110
			height: 25
			selectByMouse: true
			inputMethodHints: "ImhPreferNumbers"
		}
		Text {
			id: _modemTXLevelLabel
			x: 10
			y: 1220
			width: 80
			height: 25
			text: qsTr("TX Level")
			color: "white"
			verticalAlignment: Text.AlignVCenter
		}
		TextField {
			id: _modemTXLevelEdit
			x: 100
			y: 1220
			width: parent.width - 110
			height: 25
			selectByMouse: true
			inputMethodHints: "ImhPreferNumbers"
		}
		Text {
			id: _modemRXDCOffsetLabel
			x: 10
			y: 1250
			width: 80
			height: 25
			text: qsTr("RX DC Offset")
			color: "white"
			verticalAlignment: Text.AlignVCenter
		}
		TextField {
			id: _modemRXDCOffsetEdit
			x: 100
			y: 1250
			width: parent.width - 110
			height: 25
			selectByMouse: true
			inputMethodHints: "ImhPreferNumbers"
		}
		Text {
			id: _modemTXDCOffsetLabel
			x: 10
			y: 1280
			width: 80
			height: 25
			text: qsTr("TX DC Offset")
			color: "white"
			verticalAlignment: Text.AlignVCenter
		}
		TextField {
			id: _modemTXDCOffsetEdit
			x: 100
			y: 1280
			width: parent.width - 110
			height: 25
			selectByMouse: true
			inputMethodHints: "ImhPreferNumbers"
		}
		Text {
			id: _modemRFLevelLabel
			x: 10
			y: 1310
			width: 80
			height: 25
			text: qsTr("RF Level")
			color: "white"
			verticalAlignment: Text.AlignVCenter
		}
		TextField {
			id: _modemRFLevelEdit
			x: 100
			y: 1310
			width: parent.width - 110
			height: 25
			selectByMouse: true
			inputMethodHints: "ImhPreferNumbers"
		}
		Text {
			id: _modemTXDelayLabel
			x: 10
			y: 1340
			width: 80
			height: 25
			text: qsTr("TX Delay")
			color: "white"
			verticalAlignment: Text.AlignVCenter
		}
		TextField {
			id: _modemTXDelayEdit
			x: 100
			y: 1340
			width: parent.width - 110
			height: 25
			selectByMouse: true
			inputMethodHints: "ImhPreferNumbers"
		}
		Text {
			id: _modemCWIdTXLevelLabel
			x: 10
			y: 1370
			width: 80
			height: 25
			text: qsTr("CWIdTXLevel")
			color: "white"
			verticalAlignment: Text.AlignVCenter
		}
		TextField {
			id: _modemCWIdTXLevelEdit
			x: 100
			y: 1370
			width: parent.width - 110
			height: 25
			selectByMouse: true
			inputMethodHints: "ImhPreferNumbers"
		}
		Text {
			id: _modemDStarTXLevelLabel
			x: 10
			y: 1400
			width: 80
			height: 25
			text: qsTr("DStarTXLevel")
			color: "white"
			verticalAlignment: Text.AlignVCenter
		}
		TextField {
			id: _modemDStarTXLevelEdit
			x: 100
			y: 1400
			width: parent.width - 110
			height: 25
			selectByMouse: true
			inputMethodHints: "ImhPreferNumbers"
		}
		Text {
			id: _modemDMRTXLevelLabel
			x: 10
			y: 1430
			width: 80
			height: 25
			text: qsTr("DMRTXLevel")
			color: "white"
			verticalAlignment: Text.AlignVCenter
		}
		TextField {
			id: _modemDMRTXLevelEdit
			x: 100
			y: 1430
			width: parent.width - 110
			height: 25
			selectByMouse: true
			inputMethodHints: "ImhPreferNumbers"
		}
		Text {
			id: _modemYSFTXLevelLabel
			x: 10
			y: 1460
			width: 80
			height: 25
			text: qsTr("YSFTXLevel")
			color: "white"
			verticalAlignment: Text.AlignVCenter
		}
		TextField {
			id: _modemYSFTXLevelEdit
			x: 100
			y: 1460
			width: parent.width - 110
			height: 25
			selectByMouse: true
			inputMethodHints: "ImhPreferNumbers"
		}
		Text {
			id: _modemP25TXLevelLabel
			x: 10
			y: 1490
			width: 80
			height: 25
			text: qsTr("P25TXLevel")
			color: "white"
			verticalAlignment: Text.AlignVCenter
		}
		TextField {
			id: _modemP25TXLevelEdit
			x: 100
			y: 1490
			width: parent.width - 110
			height: 25
			selectByMouse: true
			inputMethodHints: "ImhPreferNumbers"
		}
		Text {
			id: _modemNXDNTXLevelLabel
			x: 10
			y: 1520
			width: 80
			height: 25
			text: qsTr("NXDNTXLevel")
			color: "white"
			verticalAlignment: Text.AlignVCenter
		}
		TextField {
			id: _modemNXDNTXLevelEdit
			x: 100
			y: 1520
			width: parent.width - 110
			height: 25
			selectByMouse: true
			inputMethodHints: "ImhPreferNumbers"
		}
		Text {
			id: _vocoderURLlabel
			x: 10
			y: 1550
			width: 80
			height: 25
			text: qsTr("Vocoder URL")
			color: "white"
			verticalAlignment: Text.AlignVCenter
		}
		TextField {
			id: _vocoderURLEdit
			x: 100
			y: 1550
			width: parent.width - 110
			height: 25
			selectByMouse: true
		}
		Button {
			id: vocoderButton
			x: 10
			y: 1580
			width: 150
			height: 30
			text: qsTr("Download vocoder")
			onClicked: {
				droidstar.download_file(_vocoderURLEdit.text, true);
				updateDialog.open();
			}
		}
	}
}
