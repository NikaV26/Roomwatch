#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_rom_sys.h"

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"

#include "mqtt_client.h"

#define DHT_PIN GPIO_NUM_5

// WLAN DATEN
#define WIFI_SSID  "FRITZ!Box 6490 Cable"
#define WIFI_PASS  "38285221590804510527"

// MQTT BROKER
#define MQTT_URI "mqtt://broker.emqx.io"

esp_mqtt_client_handle_t client;

// ---------------- DHT22 ----------------
int dht_read(float *temp, float *hum) {
    uint8_t data[5] = {0};

    gpio_set_direction(DHT_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(DHT_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(20));

    gpio_set_level(DHT_PIN, 1);
    esp_rom_delay_us(40);
    gpio_set_direction(DHT_PIN, GPIO_MODE_INPUT);

    int timeout = 0;

    // Sensor response
    while (gpio_get_level(DHT_PIN)) {
        if (++timeout > 10000) return -1;
    }

    timeout = 0;
    while (!gpio_get_level(DHT_PIN)) {
        if (++timeout > 10000) return -1;
    }

    timeout = 0;
    while (gpio_get_level(DHT_PIN)) {
        if (++timeout > 10000) return -1;
    }

    for (int i = 0; i < 40; i++) {

        timeout = 0;
        while (!gpio_get_level(DHT_PIN)) {
            if (++timeout > 10000) return -1;
        }

        int width = 0;
        timeout = 0;

        while (gpio_get_level(DHT_PIN)) {
            width++;
            esp_rom_delay_us(1);

            if (++timeout > 10000) return -1;
        }

        data[i/8] <<= 1;
        if (width > 40) {
            data[i/8] |= 1;
        }
    }

    if ((data[0] + data[1] + data[2] + data[3]) != data[4]) {
        return -1;
    }

    *hum = ((data[0] << 8) | data[1]) / 10.0;
    *temp = ((data[2] << 8) | data[3]) / 10.0;

    return 0;
}

// ---------------- WLAN ----------------
void wifi_init(void) {
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();
    esp_wifi_connect();
}

// ---------------- MQTT ----------------
void mqtt_start(void) {
    esp_mqtt_client_config_t config = {
        .broker.address.uri = MQTT_URI,
    };

    client = esp_mqtt_client_init(&config);
    esp_mqtt_client_start(client);
}

// ---------------- MAIN ----------------
void app_main(void) {
    nvs_flash_init();

    wifi_init();

    printf("Warte auf WLAN...\n");
    vTaskDelay(pdMS_TO_TICKS(5000));
    printf("Starte MQTT...\n");

    mqtt_start();

    float t, h;

    while (1) {
        if (dht_read(&t, &h) == 0) {
            char msg[64];
            sprintf(msg, "{\"temp\": %.1f, \"hum\": %.1f}", t, h);

            printf("Sende: %s\n", msg);

            esp_mqtt_client_publish(client, "room/data", msg, 0, 1, 0);
        } else {
            //printf("Fehler beim Lesen\n");
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}