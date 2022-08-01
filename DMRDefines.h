/*
 *   Copyright (C) 2015,2016 by Jonathan Naylor G4KLX
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#if !defined(DMRDefines_H)
#define	DMRDefines_H
#include <cstdint>

const uint8_t TAG_HEADER = 0x00U;
const uint8_t TAG_DATA   = 0x01U;
const uint8_t TAG_LOST   = 0x02U;
const uint8_t TAG_EOT    = 0x03U;
const uint8_t TAG_NODATA = 0x04U;

const uint32_t DMR_FRAME_LENGTH_BITS  = 264U;
const uint32_t DMR_FRAME_LENGTH_BYTES = 33U;

const uint32_t DMR_SYNC_LENGTH_BITS  = 48U;
const uint32_t DMR_SYNC_LENGTH_BYTES = 6U;

const uint32_t DMR_EMB_LENGTH_BITS  = 8U;
const uint32_t DMR_EMB_LENGTH_BYTES = 1U;

const uint32_t DMR_SLOT_TYPE_LENGTH_BITS  = 8U;
const uint32_t DMR_SLOT_TYPE_LENGTH_BYTES = 1U;

const uint32_t DMR_EMBEDDED_SIGNALLING_LENGTH_BITS  = 32U;
const uint32_t DMR_EMBEDDED_SIGNALLING_LENGTH_BYTES = 4U;

const uint32_t DMR_AMBE_LENGTH_BITS  = 108U * 2U;
const uint32_t DMR_AMBE_LENGTH_BYTES = 27U;

const uint8_t BS_SOURCED_AUDIO_SYNC[]   = {0x07U, 0x55U, 0xFDU, 0x7DU, 0xF7U, 0x5FU, 0x70U};
const uint8_t BS_SOURCED_DATA_SYNC[]    = {0x0DU, 0xFFU, 0x57U, 0xD7U, 0x5DU, 0xF5U, 0xD0U};

const uint8_t MS_SOURCED_AUDIO_SYNC[]   = {0x07U, 0xF7U, 0xD5U, 0xDDU, 0x57U, 0xDFU, 0xD0U};
const uint8_t MS_SOURCED_DATA_SYNC[]    = {0x0DU, 0x5DU, 0x7FU, 0x77U, 0xFDU, 0x75U, 0x70U};

const uint8_t DIRECT_SLOT1_AUDIO_SYNC[] = {0x05U, 0xD5U, 0x77U, 0xF7U, 0x75U, 0x7FU, 0xF0U};
const uint8_t DIRECT_SLOT1_DATA_SYNC[]  = {0x0FU, 0x7FU, 0xDDU, 0x5DU, 0xDFU, 0xD5U, 0x50U};

const uint8_t DIRECT_SLOT2_AUDIO_SYNC[] = {0x07U, 0xDFU, 0xFDU, 0x5FU, 0x55U, 0xD5U, 0xF0U};
const uint8_t DIRECT_SLOT2_DATA_SYNC[]  = {0x0DU, 0x75U, 0x57U, 0xF5U, 0xFFU, 0x7FU, 0x50U};

const uint8_t SYNC_MASK[]               = {0x0FU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xF0U};

// The PR FILL and Data Sync pattern.
const uint8_t DMR_IDLE_DATA[] = {TAG_DATA, 0x00U,
							0x53U, 0xC2U, 0x5EU, 0xABU, 0xA8U, 0x67U, 0x1DU, 0xC7U, 0x38U, 0x3BU, 0xD9U,
							0x36U, 0x00U, 0x0DU, 0xFFU, 0x57U, 0xD7U, 0x5DU, 0xF5U, 0xD0U, 0x03U, 0xF6U,
							0xE4U, 0x65U, 0x17U, 0x1BU, 0x48U, 0xCAU, 0x6DU, 0x4FU, 0xC6U, 0x10U, 0xB4U};

// A silence frame only
const uint8_t DMR_SILENCE_DATA[] = {0xB9U, 0xE8U, 0x81U, 0x52U, 0x61U, 0x73U, 0x00U, 0x2AU, 0x6BU, 0xB9U, 0xE8U,
										0x81U, 0x52U, 0x60U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x01U, 0x73U, 0x00U,
										0x2AU, 0x6BU, 0xB9U, 0xE8U, 0x81U, 0x52U, 0x61U, 0x73U, 0x00U, 0x2AU, 0x6BU};

const uint8_t PAYLOAD_LEFT_MASK[]       = {0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xF0U};
const uint8_t PAYLOAD_RIGHT_MASK[]      = {0x0FU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU};

const uint8_t VOICE_LC_HEADER_CRC_MASK[]    = {0x96U, 0x96U, 0x96U};
const uint8_t TERMINATOR_WITH_LC_CRC_MASK[] = {0x99U, 0x99U, 0x99U};
const uint8_t PI_HEADER_CRC_MASK[]          = {0x69U, 0x69U};
const uint8_t DATA_HEADER_CRC_MASK[]        = {0xCCU, 0xCCU};
const uint8_t CSBK_CRC_MASK[]               = {0xA5U, 0xA5U};

const uint32_t DMR_SLOT_TIME = 60U;
const uint32_t AMBE_PER_SLOT = 3U;

const uint8_t DT_MASK               = 0x0FU;
const uint8_t DT_VOICE_PI_HEADER    = 0x00U;
const uint8_t DT_VOICE_LC_HEADER    = 0x01U;
const uint8_t DT_TERMINATOR_WITH_LC = 0x02U;
const uint8_t DT_CSBK               = 0x03U;
const uint8_t DT_DATA_HEADER        = 0x06U;
const uint8_t DT_RATE_12_DATA       = 0x07U;
const uint8_t DT_RATE_34_DATA       = 0x08U;
const uint8_t DT_IDLE               = 0x09U;
const uint8_t DT_RATE_1_DATA        = 0x0AU;

// Dummy values
const uint8_t DT_VOICE_SYNC  = 0xF0U;
const uint8_t DT_VOICE       = 0xF1U;

const uint8_t DMR_IDLE_RX    = 0x80U;
const uint8_t DMR_SYNC_DATA  = 0x40U;
const uint8_t DMR_SYNC_AUDIO = 0x20U;

const uint8_t DMR_SLOT1      = 0x00U;
const uint8_t DMR_SLOT2      = 0x80U;

const uint8_t DPF_UDT              = 0x00U;
const uint8_t DPF_RESPONSE         = 0x01U;
const uint8_t DPF_UNCONFIRMED_DATA = 0x02U;
const uint8_t DPF_CONFIRMED_DATA   = 0x03U;
const uint8_t DPF_DEFINED_SHORT    = 0x0DU;
const uint8_t DPF_DEFINED_RAW      = 0x0EU;
const uint8_t DPF_PROPRIETARY      = 0x0FU;

const uint8_t FID_ETSI = 0U;
const uint8_t FID_DMRA = 16U;

enum FLCO {
	FLCO_GROUP               = 0,
	FLCO_USER_USER           = 3,
	FLCO_TALKER_ALIAS_HEADER = 4,
	FLCO_TALKER_ALIAS_BLOCK1 = 5,
	FLCO_TALKER_ALIAS_BLOCK2 = 6,
	FLCO_TALKER_ALIAS_BLOCK3 = 7,
	FLCO_GPS_INFO            = 8
};

#endif
