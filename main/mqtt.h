#ifndef __MQTT_H
#define __MQTT_H

void mqtt_init();
void mqtt_send_env_data(esp_mqtt_client_handle_t client, env_sensor_data_t *data);

#endif
