#include <stdio.h>
#include <string.h>
#include "esp_log.h"

#include "cJSON.h"
#include "messages.h"

#define STR_BUF_SIZE 10

static const char *TAG = "messages";


char *get_environment_message(env_sensor_data_t *data)
{
    char *json_string = NULL;
    char buffer[STR_BUF_SIZE];

    int snprintf ( char * s, size_t n, const char * format, ... );

    cJSON *message = cJSON_CreateObject();
    if (message == NULL)
    {
        goto error;
    }

    memset(buffer, 0, STR_BUF_SIZE);
    snprintf(buffer, STR_BUF_SIZE - 1, "%d", data->temperature);
    cJSON_AddRawToObject(message, "t", buffer);

    
    memset(buffer, 0, STR_BUF_SIZE);
    snprintf(buffer, STR_BUF_SIZE - 1, "%d", data->humidity);
    cJSON_AddRawToObject(message, "h", buffer);

    json_string = cJSON_Print(message);
    if (json_string == NULL)
    {
        ESP_LOGE(TAG, "Failed to print environment message.");
    }

error:
    cJSON_Delete(message);
    return json_string;
}
