#pragma once

#include <driver/i2c.h>
#include <sys/types.h>

// Conversion mode definitions
#define MCP342X_MODE_ONESHOT 0x00
#define MCP342X_MODE_CONTINUOUS 0x10

// Channel definitions
// MCP3421 & MCP3425 have only the one channel and ignore this param
// MCP3422, MCP3423, MCP3426 & MCP3427 have two channels and treat 3 & 4 as repeats of 1 & 2 respectively
// MCP3424 & MCP3428 have all four channels
#define MCP342X_CHANNEL_1 0x00
#define MCP342X_CHANNEL_2 0x20
#define MCP342X_CHANNEL_3 0x40
#define MCP342X_CHANNEL_4 0x60
#define MCP342X_CHANNEL_MASK 0x60

// Sample size definitions - these also affect the sampling rate
// 12-bit has a max sample rate of 240sps
// 14-bit has a max sample rate of  60sps
// 16-bit has a max sample rate of  15sps
// 18-bit has a max sample rate of   3.75sps (MCP3421, MCP3422, MCP3423, MCP3424 only)
#define MCP342X_SIZE_12BIT 0x00
#define MCP342X_SIZE_14BIT 0x04
#define MCP342X_SIZE_16BIT 0x08
#define MCP342X_SIZE_18BIT 0x0C
#define MCP342X_SIZE_MASK 0x0C

// Programmable Gain definitions
#define MCP342X_GAIN_1X 0x00
#define MCP342X_GAIN_2X 0x01
#define MCP342X_GAIN_4X 0x02
#define MCP342X_GAIN_8X 0x03
#define MCP342X_GAIN_MASK 0x03

// /RDY bit definition
#define MCP342X_RDY 0x80

enum MCP342X_DATA_RATE { Bit16,
					Bit14,
					Bit12 };

class MCP342X {
    public:
	MCP342X(i2c_port_t port, uint8_t address, MCP342X_DATA_RATE = Bit16);
	esp_err_t getValues(int32_t* ch1 = nullptr, int32_t* ch2 = nullptr, int32_t* ch3 = nullptr, int32_t* ch4 = nullptr);

	static const uint8_t dataRateFlag[];
	static const uint16_t dataRateMask[];
	static const uint32_t dataRateCoding[];
	static const TickType_t dataRateTick[];

    private:
	uint8_t address;
	uint8_t config;
	i2c_port_t port;

	MCP342X_DATA_RATE dataRate;

	esp_err_t writeByte(uint8_t data);
	esp_err_t readBytes(uint8_t* configuration, uint8_t* buffer, size_t count);
};
