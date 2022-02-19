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
import QtQuick.Window 2.10
import QtQuick.Controls 2.3
import QtQuick.Dialogs 1.3
import org.dudetronics.droidstar 1.0

ApplicationWindow {
	// @disable-check M16
	visible: true
	 // @disable-check M16
	width: 340
	 // @disable-check M16
	height: 480
	 // @disable-check M16
	 // @disable-check M16
	title: qsTr("DroidStar")

	palette.window: "#252424"
	palette.button: "#252424"
	palette.buttonText: "white"
	palette.base: "black"
	palette.text: "white"
	palette.windowText: "white"
	palette.highlight: "steelblue"

	MessageDialog {
		id: idcheckDialog
		title: "Invalid credentials"
		text: "A valid callsign and DMR ID are required to use Dudestar on any mode, and they must match. If you have entered a valid DMR ID that matches the entered callsign, and you are still seeing this message, then you either have to click update ID files button or wait until your DMR ID is added to the ID file and try again."
	}
	MessageDialog {
		id: errorDialog
		title: "Error"
	}
	MessageDialog {
		id: updateDialog
		title: "Updating..."
		text: "Check log tab for details"
	}
	MessageDialog {
		id: vocoderDialog
		title: "No vocoder found"
		text: "No hardware or software vocoder found for this mode. You can still connect, but you will not RX or TX any audio. See the project website (url on the About tab) for info on loading a sw vocoder, or use a USB AMBE dongle (and an OTG adapter on Android devices)"
	}

	TabBar {
		id: bar
		width: parent.width
		currentIndex: swiper.currentIndex
		background: Rectangle {
			color: "steelblue"
		}
		TabButton {
			id: mainButton
			padding: 10
			background: Rectangle {
				color: bar.currentIndex === 0 ? "steelblue" : "#353535"
			}
			text: qsTr("Main")
		}
		TabButton {
			id: settingsButton
			padding: 10
			background: Rectangle {
				color: bar.currentIndex === 1 ? "steelblue" : "#353535"
			}
			text: qsTr("Settings")
		}
		TabButton {
			id: logButton
			padding: 10
			background: Rectangle {
				color: bar.currentIndex === 2 ? "steelblue" : "#353535"
			}
			text: qsTr("Log")
		}
		TabButton {
			id: hostsButton
			padding: 10
			background: Rectangle {
				color: bar.currentIndex === 3 ? "steelblue" : "#353535"
			}
			text: qsTr("Hosts")
		}
		TabButton {
			id: aboutButton
			padding: 10
			background: Rectangle {
				color: bar.currentIndex === 4 ? "steelblue" : "#353535"
			}
			text: qsTr("About")
		}
	}
	SwipeView {
		id: swiper
		width: parent.width
		height: parent.height - 50
		x: 0
		y: 50
		currentIndex: bar.currentIndex
		interactive: false

		MainTab{
			id: mainTab
		}
		SettingsTab{
			id: settingsTab
		}
		LogTab{
			id: logTab
		}
		HostsTab{
			id: hostsTab
		}
		AboutTab{}
	}
    DroidStar {
        id: droidstar
    }
	Connections {
		target: Qt.application
		function onStateChanged() {
			if (Qt.application.state !== Qt.ApplicationActive) {
				droidstar.reset_connect_status();
			}
		}
	}
    Connections {
        target: droidstar
		Component.onCompleted: {
			mainTab.comboMode.loaded = true;
			droidstar.process_settings();
			settingsTab.comboVocoder.model = droidstar.get_vocoders();
			settingsTab.comboModem.model = droidstar.get_modems();
			settingsTab.comboPlayback.model = droidstar.get_playbacks();
			settingsTab.comboCapture.model = droidstar.get_captures();
			mainTab.data1.font.family = droidstar.get_monofont();
			mainTab.data2.font.family = droidstar.get_monofont();
			mainTab.data3.font.family = droidstar.get_monofont();
			mainTab.data4.font.family = droidstar.get_monofont();
			mainTab.data5.font.family = droidstar.get_monofont();
			mainTab.data6.font.family = droidstar.get_monofont();
		}
		function onSwtx_state(s){
			mainTab.swtxBox.checked = s;
			mainTab.swtxBox.enabled = !s;
		}
		function onSwrx_state(s){
			mainTab.swrxBox.checked = s;
			mainTab.swrxBox.enabled = !s;
		}
		function onMycall_changed(s){
			settingsTab.mycallEdit.text = s;
		}
		function onUrcall_changed(s){
			settingsTab.urcallEdit.text = s;
		}
		function onRptr1_changed(s){
			settingsTab.rptr1Edit.text = s;
		}
		function onRptr2_changed(s){
			settingsTab.rptr2Edit.text = s;
		}

		function onMode_changed() {
			//console.log("onMode_changed ", mainTab.comboMode.find(droidstar.get_mode()), ":", droidstar.get_mode(), ":", droidstar.get_ref_host(), ":", droidstar.get_module());
			mainTab.label1.text = droidstar.get_label1();
			mainTab.label2.text = droidstar.get_label2();
			mainTab.label3.text = droidstar.get_label3();
			mainTab.label4.text = droidstar.get_label4();
			mainTab.label5.text = droidstar.get_label5();
			mainTab.label6.text = droidstar.get_label6();
            droidstar.set_modelchange(true);
			mainTab.comboHost.model = droidstar.get_hosts();
            droidstar.set_modelchange(false);
			mainTab.comboMode.currentIndex = mainTab.comboMode.find(droidstar.get_mode());
            if(droidstar.get_mode() === "REF"){
				//mainTab.comboMode.width = mainTab.width / 2;
				mainTab.comboHost.visible = true;
				mainTab.dtmflabel.visible = false;
				mainTab.editIAXDTMF.visible = false;
				mainTab.dtmfsendbutton.visible = false;
				mainTab.comboHost.currentIndex = mainTab.comboHost.find(droidstar.get_ref_host());
				mainTab.comboModule.visible = true;
				mainTab.comboSlot.visible = false;
				mainTab.comboCC.visible = false;
				mainTab.element3.visible = false;
				mainTab.dmrtgidEdit.visible = false;
				mainTab.comboM17CAN.visible = false;
				mainTab.privateBox.visible = false;
				mainTab.sliderMicGain.value = 0.0;
            }
            if(droidstar.get_mode() === "DCS"){
				//mainTab.comboMode.width = mainTab.width / 2;
				mainTab.comboHost.visible = true;
				mainTab.dtmflabel.visible = false;
				mainTab.editIAXDTMF.visible = false;
				mainTab.dtmfsendbutton.visible = false;
				mainTab.comboHost.currentIndex = mainTab.comboHost.find(droidstar.get_dcs_host());
				mainTab.comboModule.visible = true;
				mainTab.comboSlot.visible = false;
				mainTab.comboCC.visible = false;
				mainTab.element3.visible = false;
				mainTab.dmrtgidEdit.visible = false;
				mainTab.comboM17CAN.visible = false;
				mainTab.privateBox.visible = false;
				mainTab.sliderMicGain.value = 0.0;
            }
            if(droidstar.get_mode() === "XRF"){
				//mainTab.comboMode.width = mainTab.width / 2;
				mainTab.comboHost.visible = true;
				mainTab.dtmflabel.visible = false;
				mainTab.editIAXDTMF.visible = false;
				mainTab.dtmfsendbutton.visible = false;
				mainTab.comboHost.currentIndex = mainTab.comboHost.find(droidstar.get_xrf_host());
				mainTab.comboModule.visible = true;
				mainTab.comboSlot.visible = false;
				mainTab.comboCC.visible = false;
				mainTab.element3.visible = false;
				mainTab.dmrtgidEdit.visible = false;
				mainTab.comboM17CAN.visible = false;
				mainTab.privateBox.visible = false;
				mainTab.sliderMicGain.value = 0.0;
            }
            if(droidstar.get_mode() === "YSF"){
				//mainTab.comboMode.width = mainTab.width / 2;
				mainTab.comboHost.visible = true;
				mainTab.dtmflabel.visible = false;
				mainTab.editIAXDTMF.visible = false;
				mainTab.dtmfsendbutton.visible = false;
				mainTab.comboHost.currentIndex = mainTab.comboHost.find(droidstar.get_ysf_host());
				mainTab.comboModule.visible = false;
				mainTab.comboSlot.visible = false;
				mainTab.comboCC.visible = false;
				mainTab.element3.visible = false;
				mainTab.dmrtgidEdit.visible = false;
				mainTab.comboM17CAN.visible = false;
				mainTab.privateBox.visible = false;
				mainTab.sliderMicGain.value = 0.5;
            }
			if(droidstar.get_mode() === "FCS"){
				//mainTab.comboMode.width = mainTab.width / 2;
				mainTab.comboHost.visible = true;
				mainTab.dtmflabel.visible = false;
				mainTab.editIAXDTMF.visible = false;
				mainTab.dtmfsendbutton.visible = false;
				mainTab.comboHost.currentIndex = mainTab.comboHost.find(droidstar.get_fcs_host());
				mainTab.comboModule.visible = false;
				mainTab.comboSlot.visible = false;
				mainTab.comboCC.visible = false;
				mainTab.element3.visible = false;
				mainTab.dmrtgidEdit.visible = false;
				mainTab.comboM17CAN.visible = false;
				mainTab.privateBox.visible = false;
				mainTab.sliderMicGain.value = 0.5;
			}
            if(droidstar.get_mode() === "DMR"){
				//mainTab.comboMode.width = (mainTab.width / 5) - 5;
				mainTab.comboHost.visible = true;
				mainTab.dtmflabel.visible = false;
				mainTab.editIAXDTMF.visible = false;
				mainTab.dtmfsendbutton.visible = false;
				mainTab.comboHost.currentIndex = mainTab.comboHost.find(droidstar.get_dmr_host());
				mainTab.comboModule.visible = false;
				mainTab.comboSlot.visible = true;
				mainTab.comboCC.visible = true;
				mainTab.element3.text = "TGID";
				mainTab.element3.visible = true;
				mainTab.dmrtgidEdit.visible = true;
				mainTab.comboM17CAN.visible = false;
				mainTab.privateBox.visible = true;
				mainTab.sliderMicGain.value = 0.5;
            }
            if(droidstar.get_mode() === "P25"){
				//mainTab.comboMode.width = mainTab.width / 2;
				mainTab.comboHost.visible = true;
				mainTab.dtmflabel.visible = false;
				mainTab.editIAXDTMF.visible = false;
				mainTab.dtmfsendbutton.visible = false;
				mainTab.comboHost.currentIndex = mainTab.comboHost.find(droidstar.get_p25_host());
				mainTab.comboModule.visible = false;
				mainTab.comboSlot.visible = false;
				mainTab.comboCC.visible = false;
				mainTab.element3.text = "TGID";
				mainTab.element3.visible = true;
				mainTab.dmrtgidEdit.visible = true;
				mainTab.comboM17CAN.visible = false;
				mainTab.privateBox.visible = false;
				mainTab.sliderMicGain.value = 0.5;
            }
            if(droidstar.get_mode() === "NXDN"){
				//mainTab.comboMode.width = mainTab.width / 2;
				mainTab.comboHost.visible = true;
				mainTab.dtmflabel.visible = false;
				mainTab.editIAXDTMF.visible = false;
				mainTab.dtmfsendbutton.visible = false;
				mainTab.comboHost.currentIndex = mainTab.comboHost.find(droidstar.get_nxdn_host());
				mainTab.comboModule.visible = false;
				mainTab.comboSlot.visible = false;
				mainTab.comboCC.visible = false;
				mainTab.element3.visible = false;
				mainTab.dmrtgidEdit.visible = false;
				mainTab.comboM17CAN.visible = false;
				mainTab.privateBox.visible = false;
				mainTab.sliderMicGain.value = 0.5;
            }
			if(droidstar.get_mode() === "M17"){
				//mainTab.comboMode.width = mainTab.width / 2;
				mainTab.comboHost.visible = true;
				mainTab.dtmflabel.visible = false;
				mainTab.editIAXDTMF.visible = false;
				mainTab.dtmfsendbutton.visible = false;
				mainTab.comboHost.currentIndex = mainTab.comboHost.find(droidstar.get_m17_host());
				mainTab.comboModule.currentIndex = mainTab.comboModule.find(droidstar.get_module());
				mainTab.comboModule.visible = true;
				mainTab.comboSlot.visible = false;
				mainTab.comboCC.visible = false;
				mainTab.element3.text = "CAN";
				mainTab.element3.visible = true;
				mainTab.dmrtgidEdit.visible = false;
				mainTab.comboM17CAN.visible = true;
				mainTab.privateBox.visible = false;
				mainTab.sliderMicGain.value = 0.5;
			}
			if(droidstar.get_mode() === "IAX"){
				//mainTab.comboMode.width = mainTab.width / 2;
				mainTab.comboHost.visible = false;
				mainTab.dtmflabel.visible = true;
				mainTab.editIAXDTMF.visible = true;
				mainTab.dtmfsendbutton.visible = true;
				mainTab.comboModule.visible = false;
				mainTab.comboSlot.visible = false;
				mainTab.comboCC.visible = false;
				mainTab.element3.visible = false;
				mainTab.dmrtgidEdit.visible = false;
				mainTab.comboM17CAN.visible = false;
				mainTab.privateBox.visible = false;
				mainTab.sliderMicGain.value = 0.5;
			}
        }
		function onUpdate_data() {
			mainTab.data1.text = droidstar.get_data1();
			mainTab.data2.text = droidstar.get_data2();
			mainTab.data3.text = droidstar.get_data3();
			mainTab.data4.text = droidstar.get_data4();
			mainTab.data5.text = droidstar.get_data5();
			mainTab.data6.text = droidstar.get_data6();
			mainTab.ambestatus.text = droidstar.get_ambestatustxt();
			mainTab.mmdvmstatus.text = droidstar.get_mmdvmstatustxt();
			mainTab.netstatus.text = droidstar.get_netstatustxt();
			++mainTab.uitimer.rxcnt;
        }
		function onUpdate_settings() {
			//console.log("update_settings comboHost == ", mainTab.comboHost.find(droidstar.get_host()));
			//console.log("update_settings comboModule == ", mainTab.comboModule.find(droidstar.get_module()));
			settingsTab.ipv6.checked = droidstar.get_ipv6();
			settingsTab.xrf2ref.checked = droidstar.get_xrf2ref();
			settingsTab.toggleTX.checked = droidstar.get_toggletx();
            if(droidstar.get_mode() === "REF"){
				mainTab.comboHost.currentIndex = mainTab.comboHost.find(droidstar.get_ref_host());
            }
            if(droidstar.get_mode() === "DCS"){
				mainTab.comboHost.currentIndex = mainTab.comboHost.find(droidstar.get_dcs_host());
            }
            if(droidstar.get_mode() === "XRF"){
				mainTab.comboHost.currentIndex = mainTab.comboHost.find(droidstar.get_xrf_host());
            }
            if(droidstar.get_mode() === "YSF"){
				mainTab.comboHost.currentIndex = mainTab.comboHost.find(droidstar.get_ysf_host());
            }
			if(droidstar.get_mode() === "FCS"){
				mainTab.comboHost.currentIndex = mainTab.comboHost.find(droidstar.get_fcs_host());
			}
            if(droidstar.get_mode() === "DMR"){
				mainTab.comboHost.currentIndex = mainTab.comboHost.find(droidstar.get_dmr_host());
            }
            if(droidstar.get_mode() === "P25"){
				mainTab.comboHost.currentIndex = mainTab.comboHost.find(droidstar.get_p25_host());
            }
            if(droidstar.get_mode() === "NXDN"){
				mainTab.comboHost.currentIndex = mainTab.comboHost.find(droidstar.get_nxdn_host());
            }
			if(droidstar.get_mode() === "M17"){
				mainTab.comboHost.currentIndex = mainTab.comboHost.find(droidstar.get_m17_host());
			}
			mainTab.comboModule.currentIndex = mainTab.comboModule.find(droidstar.get_module());
			settingsTab.callsignEdit.text = droidstar.get_callsign();
			settingsTab.dmridEdit.text = droidstar.get_dmrid();
			settingsTab.comboEssid.currentIndex = settingsTab.comboEssid.find(droidstar.get_essid());
			settingsTab.bmpwEdit.text = droidstar.get_bm_password();
			settingsTab.tgifpwEdit.text = droidstar.get_tgif_password();
			settingsTab.latEdit.text = droidstar.get_latitude();
			settingsTab.lonEdit.text = droidstar.get_longitude();
			settingsTab.locEdit.text = droidstar.get_location();
			settingsTab.descEdit.text = droidstar.get_description();
			settingsTab.urlEdit.text = droidstar.get_url();
			settingsTab.swidEdit.text = droidstar.get_swid();
			settingsTab.pkgidEdit.text = droidstar.get_pkgid();
			settingsTab.dmroptsEdit.text = droidstar.get_dmr_options();
			mainTab.dmrtgidEdit.text = droidstar.get_dmrtgid();
			settingsTab.iaxuserEdit.text = droidstar.get_iax_user();
			settingsTab.iaxpassEdit.text = droidstar.get_iax_pass();
			settingsTab.iaxnodeEdit.text = droidstar.get_iax_node();
			settingsTab.iaxhostEdit.text = droidstar.get_iax_host();
			settingsTab.iaxportEdit.text = droidstar.get_iax_port();
			settingsTab.mycallEdit.text = droidstar.get_mycall();
			settingsTab.urcallEdit.text = droidstar.get_urcall();
			settingsTab.rptr1Edit.text = droidstar.get_rptr1();
			settingsTab.rptr2Edit.text = droidstar.get_rptr2();
			settingsTab.txtimerEdit.text = droidstar.get_txtimeout();

			settingsTab.modemRXFreqEdit.text = droidstar.get_modemRxFreq();
			settingsTab.modemTXFreqEdit.text = droidstar.get_modemTxFreq();
			settingsTab.modemRXOffsetEdit.text = droidstar.get_modemRxOffset();
			settingsTab.modemTXOffsetEdit.text = droidstar.get_modemTxOffset();
			settingsTab.modemRXDCOffsetEdit.text = droidstar.get_modemRxDCOffset();
			settingsTab.modemTXDCOffsetEdit.text = droidstar.get_modemTxDCOffset();
			settingsTab.modemRXLevelEdit.text = droidstar.get_modemRxLevel();
			settingsTab.modemTXLevelEdit.text = droidstar.get_modemTxLevel();
			settingsTab.modemRFLevelEdit.text = droidstar.get_modemRFLevel();
			settingsTab.modemTXDelayEdit.text = droidstar.get_modemTxDelay();
			settingsTab.modemCWIdTXLevelEdit.text = droidstar.get_modemCWIdTxLevel();
			settingsTab.modemDStarTXLevelEdit.text = droidstar.get_modemDstarTxLevel();
			settingsTab.modemDMRTXLevelEdit.text = droidstar.get_modemDMRTxLevel();
			settingsTab.modemYSFTXLevelEdit.text = droidstar.get_modemYSFTxLevel();
			settingsTab.modemP25TXLevelEdit.text = droidstar.get_modemP25TxLevel()
			settingsTab.modemNXDNTXLevelEdit.text = droidstar.get_modemNXDNTxLevel();
			settingsTab.modemBaudEdit.text = droidstar.get_modemBaud();

			hostsTab.hostsTextEdit.text = droidstar.get_local_hosts();
        }
		function onUpdate_log(s) {
			logTab.logText.append(s);
		}
		function onOpen_vocoder_dialog() {
			vocoderDialog.open();
		}

		function onConnect_status_changed(c) {
			if(c === 0){
				if(mainTab.buttonTX.tx){
					mainTab.buttonTX.tx = false;
					droidstar.tx_clicked(false);
					mainTab.txtimer.running = false;
					mainTab.btntxt.color = "black";
					mainTab.btntxt.text = "TX";
				}
				mainTab.connectbutton.text = "Connect";
				mainTab.comboMode.enabled = true;
				mainTab.comboHost.enabled = true;
				mainTab.comboModule.enabled = true;
				mainTab.buttonTX.enabled = false;
				mainTab.btntxt.color = "steelblue";
				mainTab.data1.text = "";
				mainTab.data2.text = "";
				mainTab.data3.text = "";
				mainTab.data4.text = "";
				mainTab.data5.text = "";
				mainTab.data6.text = "";
				mainTab.netstatus.text = "Not connected";
            }
			if(c === 1){
				mainTab.connectbutton.text = "Connecting";
				mainTab.comboMode.enabled = false;
				mainTab.comboHost.enabled = false;
				if(mainTab.comboMode.currentText != "REF"){
					mainTab.comboModule.enabled = false;
				}
            }
			if(c === 2){
				mainTab.connectbutton.text = "Disconnect";
				mainTab.comboMode.enabled = false;
				mainTab.comboHost.enabled = false;

				if(mainTab.comboMode.currentText != "REF"){
					mainTab.comboModule.enabled = false;
				}
				if(mainTab.comboMode.currentText === "YSF"){
					settingsTab.m171600.checked = true;
				}
				if(mainTab.comboMode.currentText === "FCS"){
					settingsTab.m171600.checked = true;
				}
				if(mainTab.comboMode.currentText === "M17"){
					settingsTab.m173200.checked = true;
				}

				mainTab.buttonTX.enabled = true;
				mainTab.btntxt.color = "black";
				mainTab.agcBox.checked = true;
			}
			if(c === 3){
			}
			if(c === 4){
				idcheckDialog.open();
				onConnect_status_changed(0);
			}
			if(c === 5){
				errorDialog.text = droidstar.get_error_text();
				errorDialog.open();
				droidstar.onConnect_status_changed(0);
			}
		}
	}
}
