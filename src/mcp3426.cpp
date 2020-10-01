#include "mcp3426.hpp"

#define TAG "MCP3426 DRIVER"
#define LOG_LOCAL_LEVEL 3
#include "log.h"

/******************************************
 * Specific address constructor.
 * @param address I2C address
 * @see MCP342X_DEFAULT_ADDRESS
 * @see MCP342X_A0GND_A1GND, etc.
 */
MCP342X::MCP342X(i2c_port_t port, uint8_t address) {
	this->address = address;
	this->port    = port;
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
		err |= writeByte(0x88 | (i << 5));
		do {
			vTaskDelay(300 / portTICK_RATE_MS);
			err |= readBytes(&configuration, buffer, 2);
			_i("%d: %2x %2x %2x", err, buffer[0], buffer[1], configuration);
		} while (configuration & MCP342X_RDY);
		*channel2 = ((buffer[0] & 0b10000000) ? 0xffff0000 : 0x00000000) | (buffer[0] << 8) | buffer[1];
	}

	return err;
}
