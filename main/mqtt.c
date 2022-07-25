
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"

#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"

#include "esp_log.h"
#include "mqtt_client.h"
#include "esp_tls.h"
#include "esp_ota_ops.h"

#include "messages.h"

#define MQTT_CONNECTED_BIT BIT0
#define MQTT_CONTROL_TOPIC "env-sensor/control"
#define MQTT_DATA_TOPIC "env-sensor/data"


static const char *TAG = "mqtt";
static QueueHandle_t mqtt_queue;


#if CONFIG_BROKER_CERTIFICATE_OVERRIDDEN == 1
static const uint8_t mqtt_aws_iot_pem_start[]  = "-----BEGIN CERTIFICATE-----\n" CONFIG_BROKER_CERTIFICATE_OVERRIDE "\n-----END CERTIFICATE-----";
#else
extern const uint8_t mqtt_aws_iot_pem_start[]   asm("_binary_mqtt_aws_iot_pem_start");
#endif
extern const uint8_t mqtt_aws_iot_pem_end[]   asm("_binary_mqtt_aws_iot_pem_end");
extern const uint8_t mqtt_aws_iot_client_key_start[]	asm("_binary_mqtt_aws_iot_client_key_start");
extern const uint8_t mqtt_aws_iot_client_key_end[]	asm("_binary_mqtt_aws_iot_client_key_end");
extern const uint8_t mqtt_aws_iot_client_pem_start[]	asm("_binary_mqtt_aws_iot_client_pem_start");
extern const uint8_t mqtt_aws_iot_client_pem_end[]	asm("_binary_mqtt_aws_iot_client_pem_end");


static EventGroupHandle_t mqtt_event_group;


static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    // your_context_t *context = event->context;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            xEventGroupSetBits(mqtt_event_group, MQTT_CONNECTED_BIT);
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            esp_mqtt_client_reconnect(client);
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            msg_id = esp_mqtt_client_publish(client, "env-sensor/data", "data", 0, 0, 0);
            ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
            break;

        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            if (event->error_handle->error_type == MQTT_ERROR_TYPE_ESP_TLS) {
                ESP_LOGI(TAG, "Last error code reported from esp-tls: 0x%x", event->error_handle->esp_tls_last_esp_err);
                ESP_LOGI(TAG, "Last tls stack error number: 0x%x", event->error_handle->esp_tls_stack_err);
            } else if (event->error_handle->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED) {
                ESP_LOGI(TAG, "Connection refused error: 0x%x", event->error_handle->connect_return_code);
            } else {
                ESP_LOGW(TAG, "Unknown error type: 0x%x", event->error_handle->error_type);
            }
            break;
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
    return ESP_OK;
}


static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    mqtt_event_handler_cb(event_data);
}

void mqtt_init(esp_mqtt_client_handle_t *client)
{
    mqtt_event_group = xEventGroupCreate();

    const esp_mqtt_client_config_t mqtt_cfg = {
        .uri = CONFIG_BROKER_URI,  
        .client_id = "env-sensor-01",
        .cert_pem = (const char *)mqtt_aws_iot_pem_start,
        .client_key_pem = (const char *)mqtt_aws_iot_client_key_start,
        .client_cert_pem = (const char *)mqtt_aws_iot_client_pem_start,
    };

    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());

    *client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(*client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
    esp_mqtt_client_start(*client);

    xEventGroupWaitBits(mqtt_event_group, MQTT_CONNECTED_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
    vEventGroupDelete(mqtt_event_group);
}

void mqtt_listen_control(esp_mqtt_client_handle_t client, QueueHandle_t *queue)
{
    *queue = xQueueCreate(3, sizeof(int));
    if (*queue == NULL)
    {
        ESP_LOGE(TAG, "Cannot create MQTT message queue.");
        return;
    }

    mqtt_queue = *queue;
    esp_mqtt_client_subscribe(client, MQTT_CONTROL_TOPIC, 0);
}

void mqtt_send_env_data(esp_mqtt_client_handle_t client, env_sensor_data_t *data)
{
    char *message = get_environment_message(data);
    if (message == NULL)
    {
        ESP_LOGE(TAG, "Could not construct environment data message.");
        return;
    }

    esp_mqtt_client_publish(client, MQTT_DATA_TOPIC, message, 0, 0, 0);
    free(message);
}