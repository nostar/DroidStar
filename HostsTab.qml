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

import QtQuick
import QtQuick.Controls

Item {
	id: hostsTab
	property alias hostsTextEdit: hostsTxtEdit
	Rectangle{
		id: hostsList
		x: 20
		y: 20
		width: parent.width - 40
		height: parent.height - 40
		color: "#252424"
		Flickable{
			anchors.fill: parent
			contentWidth: parent.width
			contentHeight: hostsTxtEdit.y +
						   hostsTxtEdit.height
			flickableDirection: Flickable.VerticalFlick
			clip: true
			Text {
				id: hostsText
				width: hostsText.width
				wrapMode: Text.WordWrap
				color: "white"
				text: qsTr("Custom hostfile format:\n" +
                           "<mode> <name> <host> <port> <username (optional)> <password (optional)>\n" +
						   "Example: REF REF123 192.168.1.1 20001\n" +
                           "Example: DMR MyNet 192.168.1.1 62030 passw0rd\n" +
                           "Example: IAX 12345 192.168.1.1 4569 iaxclient iaxpass")
			}
			TextArea {
				id: hostsTxtEdit
				x: 0
				y: hostsText.height + 5
				width: hostsList.width
				height: 500
				background: Rectangle {
					color: "#000000"
					radius: 5
				}
				wrapMode: TextArea.WordWrap
				text: qsTr("")
				onEditingFinished: {
					droidstar.update_custom_hosts(hostsTxtEdit.text);
				}
			}
		}
	}
}
