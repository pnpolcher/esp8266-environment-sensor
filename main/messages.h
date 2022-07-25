#ifndef __MESSAGES_H
#define __MESSAGES_H

typedef struct env_sensor_data_t {
    int temperature;
    int humidity;
} env_sensor_data_t;

char *get_environment_message(env_sensor_data_t *data);

#endif
