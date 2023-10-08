idf_component_register(SRCS "main.c"
                       INCLUDE_DIRS "."
                       REQUIRES ahrs mpu9250
                       REQUIRES esp_wifi
                       REQUIRES nvs_flash)
