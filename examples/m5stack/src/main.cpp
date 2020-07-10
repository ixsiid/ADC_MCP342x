#include <driver/i2c.h>
#include <esp_err.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <ili9341.hpp>
#include <mcp3426.hpp>

#define TAG "MCP3426"
#include "log.h"

extern "C" {
void app_main();
}

MCP342X *adc;
esp_err_t err;

#define I2C_EXAMPLE_MASTER_SCL_IO ((gpio_num_t)22) /*!< gpio number for I2C master clock */
#define I2C_EXAMPLE_MASTER_SDA_IO ((gpio_num_t)21) /*!< gpio number for I2C master data  */
#define I2C_EXAMPLE_MASTER_TX_BUF_DISABLE 0		 /*!< I2C master do not need buffer */
#define I2C_EXAMPLE_MASTER_RX_BUF_DISABLE 0		 /*!< I2C master do not need buffer */
#define I2C_EXAMPLE_MASTER_FREQ_HZ 100000		 /*!< I2C master clock frequency */

esp_err_t initI2C(i2c_port_t i2c_port) {
	i2c_config_t conf;
	conf.mode			  = I2C_MODE_MASTER;
	conf.sda_io_num	  = I2C_EXAMPLE_MASTER_SDA_IO;
	conf.sda_pullup_en	  = GPIO_PULLUP_ENABLE;
	conf.scl_io_num	  = I2C_EXAMPLE_MASTER_SCL_IO;
	conf.scl_pullup_en	  = GPIO_PULLUP_ENABLE;
	conf.master.clk_speed = I2C_EXAMPLE_MASTER_FREQ_HZ;
	ESP_ERROR_CHECK(i2c_param_config(i2c_port, &conf));
	ESP_ERROR_CHECK(i2c_driver_install(i2c_port, conf.mode,
								I2C_EXAMPLE_MASTER_RX_BUF_DISABLE,
								I2C_EXAMPLE_MASTER_TX_BUF_DISABLE, 0));

	return i2c_set_timeout(i2c_port, 0xFFFFF);
}

void app_main() {
	LCD::ILI9341 *lcd = new LCD::ILI9341();

	err = initI2C(i2c_port_t::I2C_NUM_0);

	adc = new MCP342X(i2c_port_t::I2C_NUM_0, 0x68);

	xTaskCreatePinnedToCore([](void *lcd_p) {
		LCD::ILI9341 *lcd = (LCD::ILI9341 *)lcd_p;

		char buf[64];

		esp_err_t e;

		size_t index = 0;
		int32_t s1, s2;
		uint8_t p1[320], p2[320];
		for (int i = 0; i < 320; i++) p1[i] = p2[i] = 230;

		uint8_t r = 0;

		uint16_t *frameBuffer = lcd->getBuffer();
		buf[0] = '\0';

		while (true) {
			vTaskDelay(100);

			lcd->clear(LCD::BLACK);
			for (int x = 0; x < 320; x++)
				for (int y = 0; y < 30; y++)
					frameBuffer[y * 320 + x] = LCD::WHITE;

			e = adc->getValues(&s1, &s2);
			if (e == ESP_OK) {
				sprintf(buf, "CH1: %7d  CH2: %7d E:%d   ", s1, s2, e);
				p1[index] = 230 - (s1 / 100);
				p2[index] = 230 - (s2 / 100);
				if (p1[index] >= 240) p1[index] = 239;
				if (p1[index] <= 30) p1[index] = 31;
				if (p2[index] >= 240) p2[index] = 239;
				if (p2[index] <= 30) p2[index] = 31;
				
				index++;
				while (index >= 320) index -= 320;
			}
			lcd->drawString(5, 5, LCD::BLACK, buf);

			for (int i = 0; i < 320; i++) {
				size_t n = index + i;
				while (n >= 320) n -= 320;
				frameBuffer[p2[n] * 320 + i] = LCD::GREEN;
				frameBuffer[p1[n] * 320 + i] = LCD::RED;
			}

			r = 1 - r;
			if (r) {
				for (int x = 300; x < 320; x++) {
					for (int y = 0; y < 20; y++) frameBuffer[y * 320 + x] = 0xf800;
				}
			}

			lcd->update();
		}
	}, "MCP3426", 4096, lcd, 1, NULL, 1);
}
