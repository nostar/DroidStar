#ifndef MMDVMDEFINES_H
#define MMDVMDEFINES_H
#include <cstdint>

enum HW_TYPE {
	HWT_MMDVM,
	HWT_DVMEGA,
	HWT_MMDVM_ZUMSPOT,
	HWT_MMDVM_HS_HAT,
	HWT_NANO_HOTSPOT,
	HWT_MMDVM_HS,
	HWT_UNKNOWN
};

enum RPT_RF_STATE {
	RS_RF_LISTENING,
	RS_RF_LATE_ENTRY,
	RS_RF_AUDIO,
	RS_RF_DATA,
	RS_RF_REJECTED,
	RS_RF_INVALID
};

enum RPT_NET_STATE {
	RS_NET_IDLE,
	RS_NET_AUDIO,
	RS_NET_DATA
};

enum B_STATUS {
	BS_NO_DATA,
	BS_DATA,
	BS_MISSING
};

const uint8_t MMDVM_FRAME_START = 0xE0U;

const uint8_t MMDVM_GET_VERSION = 0x00U;
const uint8_t MMDVM_GET_STATUS  = 0x01U;
const uint8_t MMDVM_SET_CONFIG  = 0x02U;
const uint8_t MMDVM_SET_MODE    = 0x03U;
const uint8_t MMDVM_SET_FREQ    = 0x04U;

const uint8_t MMDVM_SEND_CWID   = 0x0AU;

const uint8_t MMDVM_DSTAR_HEADER = 0x10U;
const uint8_t MMDVM_DSTAR_DATA   = 0x11U;
const uint8_t MMDVM_DSTAR_LOST   = 0x12U;
const uint8_t MMDVM_DSTAR_EOT    = 0x13U;

const uint8_t MMDVM_DMR_DATA1   = 0x18U;
const uint8_t MMDVM_DMR_LOST1   = 0x19U;
const uint8_t MMDVM_DMR_DATA2   = 0x1AU;
const uint8_t MMDVM_DMR_LOST2   = 0x1BU;
const uint8_t MMDVM_DMR_SHORTLC = 0x1CU;
const uint8_t MMDVM_DMR_START   = 0x1DU;
const uint8_t MMDVM_DMR_ABORT   = 0x1EU;

const uint8_t MMDVM_YSF_DATA    = 0x20U;
const uint8_t MMDVM_YSF_LOST    = 0x21U;

const uint8_t MMDVM_P25_HDR     = 0x30U;
const uint8_t MMDVM_P25_LDU     = 0x31U;
const uint8_t MMDVM_P25_LOST    = 0x32U;

const uint8_t MMDVM_NXDN_DATA   = 0x40U;
const uint8_t MMDVM_NXDN_LOST   = 0x41U;

const uint8_t MMDVM_M17_LINK_SETUP = 0x45U;
const uint8_t MMDVM_M17_STREAM     = 0x46U;
const uint8_t MMDVM_M17_PACKET     = 0x47U;
const uint8_t MMDVM_M17_LOST       = 0x48U;
const uint8_t MMDVM_M17_EOT        = 0x49U;

const uint8_t MMDVM_POCSAG_DATA = 0x50U;

const uint8_t MMDVM_FM_PARAMS1  = 0x60U;
const uint8_t MMDVM_FM_PARAMS2  = 0x61U;
const uint8_t MMDVM_FM_PARAMS3  = 0x62U;

const uint8_t MMDVM_ACK         = 0x70U;
const uint8_t MMDVM_NAK         = 0x7FU;

const uint8_t MMDVM_SERIAL      = 0x80U;

const uint8_t MMDVM_TRANSPARENT = 0x90U;
const uint8_t MMDVM_QSO_INFO    = 0x91U;

const uint8_t MMDVM_DEBUG1      = 0xF1U;
const uint8_t MMDVM_DEBUG2      = 0xF2U;
const uint8_t MMDVM_DEBUG3      = 0xF3U;
const uint8_t MMDVM_DEBUG4      = 0xF4U;
const uint8_t MMDVM_DEBUG5      = 0xF5U;

const uint32_t MAX_RESPONSES = 30U;

const uint32_t BUFFER_LENGTH = 2000U;
const uint8_t MODE_IDLE    = 0U;
const uint8_t MODE_DSTAR   = 1U;
const uint8_t MODE_DMR     = 2U;
const uint8_t MODE_YSF     = 3U;
const uint8_t MODE_P25     = 4U;
const uint8_t MODE_NXDN    = 5U;
const uint8_t MODE_M17     = 7U;
const uint8_t MODE_CW      = 98U;
const uint8_t MODE_LOCKOUT = 99U;
const uint8_t MODE_ERROR   = 100U;

#endif // MMDVMDEFINES_H
