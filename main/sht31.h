#ifndef __SHT31_H
#define __SHT31_H

esp_err_t sht31_start_measurement();
esp_err_t sht31_read_measurement(uint8_t *pbResult);
esp_err_t sht31_read_single_shot(double *temperature, double *humidity);
void i2c_init();

#endif
