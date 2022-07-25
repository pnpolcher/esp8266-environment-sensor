/* WiFi station Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include "esp_system.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "mqtt_client.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "nvs.h"
#include "nvs_flash.h"

#include "messages.h"
#include "mqtt.h"
#include "sht31.h"
#include "wifi.h"

static const char *TAG = "main";
static esp_mqtt_client_handle_t client;

void vMeasureTask(void *pvParameters)
{
    env_sensor_data_t data;
    double t, h;

    for (;;)
    {
        esp_err_t err = sht31_read_single_shot(&t, &h);
        // ESP_LOGI(TAG, "Temperature = %d, humidity = %d", (int)data.temperature, (int)data.humidity);
        if (err == ESP_OK)
        {
            data.temperature = (int)(t * 1000.0f);
            data.humidity = (int)(h * 100.0f);
            mqtt_send_env_data(client, &data);
        }
    }
}

void app_main()
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());

    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    wifi_init_sta();
    mqtt_init(&client);
    i2c_init();

    BaseType_t xReturned;
    TaskHandle_t xMeasureHandle = NULL;

    xReturned = xTaskCreate(
        vMeasureTask,
        "Measure",
        2048, // stack size in words
        (void *)1, // parameters
        10, // priority,
        &xMeasureHandle
    );

    if (xReturned == pdPASS)
    {

    }
}
