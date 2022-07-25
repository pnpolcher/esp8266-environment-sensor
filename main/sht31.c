#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/i2c.h"


#define I2C_SCL_IO                  4                /*!< gpio number for I2C master clock */
#define I2C_SDA_IO                  5                   /*!< gpio number for I2C master data  */
#define I2C_MASTER_PORT_NUM         I2C_NUM_0        /*!< I2C port number for master dev */
#define I2C_TX_BUF_DISABLE          0                /*!< I2C master do not need buffer */
#define I2C_MASTER_RX_BUF_DISABLE   0                /*!< I2C master do not need buffer */
#define I2C_MASTER_TX_BUF_DISABLE   0

#define SHT31_ADDR                  0x44

esp_err_t sht31_start_measurement()
{
    uint8_t bData[3];
    i2c_cmd_handle_t cmd;
    esp_err_t err;

    bData[0] = (SHT31_ADDR << 1) | I2C_MASTER_WRITE;
    bData[1] = 0x24;
    bData[2] = 0x00;

    cmd = i2c_cmd_link_create();

    err = i2c_master_start(cmd);
    if (err != ESP_OK)
    {
        printf("SM-F: i2c_master_start\n");
        goto i2c_error;
    }

    err = i2c_master_write(cmd, bData, 3, I2C_MASTER_ACK);
    if (err != ESP_OK)
    {
        printf("SM-F: i2c_master_write\n");
        goto i2c_error;
    }
    
    err = i2c_master_stop(cmd);
    if (err != ESP_OK)
    {
        printf("SM-F: i2c_master_stop1\n");
        goto i2c_error;
    }

    err = i2c_master_cmd_begin(I2C_MASTER_PORT_NUM, cmd, 1000 / portTICK_PERIOD_MS);
    if (err != ESP_OK)
    {
        printf("SM-F: i2c_master_cmd_begin\n");
        goto i2c_error;
    }

i2c_error:
    i2c_cmd_link_delete(cmd);
    return err;
}

esp_err_t sht31_read_measurement(uint8_t *pbResult)
{
    i2c_cmd_handle_t cmd;
    esp_err_t err;

    // Start + read
    cmd = i2c_cmd_link_create();

    err = i2c_master_start(cmd);
    if (err != ESP_OK)
    {
        printf("F: i2c_master_start 2\n");
        goto i2c_error;
    }

    err = i2c_master_write_byte(cmd, (SHT31_ADDR << 1) | I2C_MASTER_READ, true);
    if (err != ESP_OK)
    {
        printf("F: i2c_master_write_byte\n");
        goto i2c_error;
    }

    err = i2c_master_read(cmd, pbResult, 9, I2C_MASTER_ACK);
    if (err != ESP_OK)
    {
        printf("F: i2c_master_read\n");
        goto i2c_error;
    }

    err = i2c_master_read(cmd, (pbResult + 9), 1, I2C_MASTER_NACK);
    if (err != ESP_OK)
    {
        printf("F: i2c_master_read2\n");
        goto i2c_error;
    }

    err = i2c_master_stop(cmd);
    if (err != ESP_OK)
    {
        printf("F: i2c_master_stop3\n");
        goto i2c_error;
    }

    err = i2c_master_cmd_begin(I2C_MASTER_PORT_NUM, cmd, 3000 / portTICK_PERIOD_MS);
    if (err != ESP_OK)
    {
        printf("F: i2c_master_cmd_begin2\n");
        goto i2c_error;
    }

i2c_error:
    i2c_cmd_link_delete(cmd);
    return err;
}

esp_err_t sht31_read_single_shot(double *temperature, double *humidity)
{
    uint8_t bResult[10];
    esp_err_t err;

    err = sht31_start_measurement();
    if (err != ESP_OK)
    {
        return err;
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS);
    
    err = sht31_read_measurement(&bResult);
    if (err != ESP_OK)
    {
        return err;
    }

    uint16_t uiTemp = bResult[0] << 8 | bResult[1];
    uint16_t uiHumidity = bResult[3] << 8 | bResult[4];

    *temperature = -45.0f + 175.0f * (double)uiTemp / 65535.0f;
    *humidity = 100.0f * (double)uiHumidity / 65535.0f;

    return ESP_OK;
}

void i2c_init()
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_SDA_IO,
        .sda_pullup_en = 0,
        .scl_io_num = I2C_SCL_IO,
        .scl_pullup_en = 0,
        .clk_stretch_tick = 300
    };

    ESP_ERROR_CHECK(i2c_driver_install(I2C_MASTER_PORT_NUM, conf.mode));
    ESP_ERROR_CHECK(
        i2c_param_config(
            I2C_MASTER_PORT_NUM,
            &conf
        )
    );
}
