idf_component_register(SRCS "main.c"
                       PRIV_REQUIRES spi_flash esp_driver_gpio esp_wifi esp_event esp_netif nvs_flash esp-mqtt
                       INCLUDE_DIRS "")