#include "mcp342x.hpp"

#define TAG "MCP342X DRIVER"
#define LOG_LOCAL_LEVEL 3
#include "log.h"

const uint8_t MCP342X::dataRateFlag[]	 = {0x80, 0x40, 0x00};
const uint16_t MCP342X::dataRateMask[]	 = {0x0000, 0x3000, 0x0800};
const uint32_t MCP342X::dataRateCoding[] = {0x00000000, 0xffffc000, 0xfffff000};
const TickType_t MCP342X::dataRateTick[] = {1000 / 15 / portTICK_RATE_MS, 1000 / 60 / portTICK_RATE_MS, 1000 / 240 / portTICK_RATE_MS};

/******************************************
 * Specific address constructor.
 * @param address I2C address
 * @see MCP342X_DEFAULT_ADDRESS
 * @see MCP342X_A0GND_A1GND, etc.
 */
MCP342X::MCP342X(i2c_port_t port, uint8_t address, MCP342X_DATA_RATE dataRate) {
	this->address	= address;
	this->port	= port;
	this->dataRate = dataRate;
}

esp_err_t MCP342X::writeByte(uint8_t data) {
	i2c_cmd_handle_t cmd;
	esp_err_t err = 0;

	cmd = i2c_cmd_link_create();
	err |= i2c_master_start(cmd);
	err |= i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_WRITE, true);
	err |= i2c_master_write_byte(cmd, data, true);

	err |= i2c_master_stop(cmd);
	err |= i2c_master_cmd_begin(port, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

	return err;
}

esp_err_t MCP342X::readBytes(uint8_t *configuration, uint8_t *buffer, size_t count) {
	i2c_cmd_handle_t cmd;
	esp_err_t err = 0;

	cmd = i2c_cmd_link_create();
	err |= i2c_master_start(cmd);
	err |= i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_READ, true);
	err |= i2c_master_read(cmd, buffer, count, i2c_ack_type_t::I2C_MASTER_ACK);
	err |= i2c_master_read_byte(cmd, configuration, i2c_ack_type_t::I2C_MASTER_NACK);

	err |= i2c_master_stop(cmd);
	err |= i2c_master_cmd_begin(port, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

	return err;
}

esp_err_t MCP342X::getValues(int32_t *channel1, int32_t *channel2, int32_t *channel3, int32_t *channel4) {
	uint8_t buffer[4];
	uint8_t configuration;

	esp_err_t err = ESP_OK;
	err |= writeByte(0x08);

	int32_t *channel[4] = {channel1, channel2, channel3, channel4};
	for (int i = 0; i < 4; i++) {
		if (err != ESP_OK) break;
		if (channel[i] == nullptr) continue;
		uint8_t a = 0x80 | dataRateFlag[dataRate] | (i << 5);
		err |= writeByte(a);
		do {
			vTaskDelay(70 / portTICK_RATE_MS);
			err |= readBytes(&configuration, buffer, 2);
			_v("%d(%2x): %2x %2x %2x", err, a, buffer[0], buffer[1], configuration);
		} while (configuration & MCP342X_RDY);

		*channel[i] = buffer[0];
		*channel[i] <<= 8;
		if (*channel[i] & dataRateMask[dataRate]) *channel[i] |= dataRateCoding[dataRate];
		*channel[i] |= buffer[1];

		_v("ReadValue: %d, %x", *(channel[i]), *(channel[i]));
	}

	return err;
}
