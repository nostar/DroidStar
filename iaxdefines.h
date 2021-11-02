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

	These macros were derived from the iaxclient library
	https://www.voip-info.org/iaxclient/
*/

#ifndef IAXDEFINES_H
#define IAXDEFINES_H

#define IAX_PROTO_VERSION			2

#define AST_FRAME_DTMF				1
#define AST_FRAME_VOICE				2
#define AST_FRAME_CONTROL			4
#define AST_FRAME_IAX				6
#define AST_FRAME_TEXT				7

#define AST_CONTROL_HANGUP			1
#define AST_CONTROL_RING			2
#define AST_CONTROL_RINGING			3
#define AST_CONTROL_ANSWER			4
#define AST_CONTROL_OPTION			11
#define AST_CONTROL_KEY				12
#define AST_CONTROL_UNKEY			13

#define AST_FORMAT_ULAW				4
#define IAX_AUTH_MD5				2

#define IAX_COMMAND_NEW				1
#define IAX_COMMAND_PING			2
#define IAX_COMMAND_PONG			3
#define IAX_COMMAND_ACK				4
#define IAX_COMMAND_HANGUP			5
#define IAX_COMMAND_REJECT			6
#define IAX_COMMAND_ACCEPT			7
#define IAX_COMMAND_AUTHREQ			8
#define IAX_COMMAND_AUTHREP			9
#define IAX_COMMAND_INVAL			10
#define IAX_COMMAND_LAGRQ			11
#define IAX_COMMAND_LAGRP			12
#define IAX_COMMAND_REGREQ			13
#define IAX_COMMAND_REGAUTH			14
#define IAX_COMMAND_REGACK			15
#define IAX_COMMAND_REGREJ			16
#define IAX_COMMAND_VNAK			18

#define IAX_IE_CALLED_NUMBER		1
#define IAX_IE_CALLING_NUMBER		2
#define IAX_IE_CALLING_NAME			4
#define IAX_IE_CALLED_CONTEXT		5
#define IAX_IE_USERNAME				6
#define IAX_IE_PASSWORD				7
#define IAX_IE_CAPABILITY			8
#define IAX_IE_FORMAT				9
#define IAX_IE_VERSION				11
#define IAX_IE_DNID					13
#define IAX_IE_AUTHMETHODS			14
#define IAX_IE_CHALLENGE			15
#define IAX_IE_MD5_RESULT			16
#define IAX_IE_APPARENT_ADDR		18
#define IAX_IE_REFRESH				19
#define IAX_IE_CAUSE				22
#define IAX_IE_DATETIME				31
#define IAX_IE_RR_JITTER            46
#define IAX_IE_RR_LOSS              47
#define IAX_IE_RR_PKTS              48
#define IAX_IE_RR_DELAY             49
#define IAX_IE_RR_DROPPED           50
#define IAX_IE_RR_OOO               51

#endif // IAXDEFINES_H
