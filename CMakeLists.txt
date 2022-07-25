# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(env_sensor)

target_add_binary_data(env_sensor.elf "main/certs/mqtt_aws_iot.pem" TEXT)
target_add_binary_data(env_sensor.elf "main/certs/mqtt_aws_iot_client.key" TEXT)
target_add_binary_data(env_sensor.elf "main/certs/mqtt_aws_iot_client.pem" TEXT)
