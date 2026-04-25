#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_rom_sys.h"

#define DHT_PIN GPIO_NUM_5

int dht_read(float *temp, float *hum) {
    uint8_t data[5] = {0};

    // Startsignal
    gpio_set_direction(DHT_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(DHT_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(20));

    gpio_set_level(DHT_PIN, 1);
    esp_rom_delay_us(40);
    gpio_set_direction(DHT_PIN, GPIO_MODE_INPUT);

    // Antwort abwarten
    int timeout = 0;
    while (gpio_get_level(DHT_PIN)) if (++timeout > 10000) return -1;
    while (!gpio_get_level(DHT_PIN));
    while (gpio_get_level(DHT_PIN));

    // 40 Bits lesen
    for (int i = 0; i < 40; i++) {
        while (!gpio_get_level(DHT_PIN));

        int width = 0;
        while (gpio_get_level(DHT_PIN)) {
            width++;
            esp_rom_delay_us(1);
        }

        data[i/8] <<= 1;
        if (width > 40) data[i/8] |= 1;
    }

    // Checksum
    if ((data[0] + data[1] + data[2] + data[3]) != data[4]) return -1;

    *hum = ((data[0] << 8) | data[1]) / 10.0;
    *temp = ((data[2] << 8) | data[3]) / 10.0;

    return 0;
}

void app_main(void) {
    float t, h;

    while (1) {
        if (dht_read(&t, &h) == 0) {
            printf("Temp: %.1f °C | Hum: %.1f %%\n", t, h);
        } else {
            printf("Fehler beim Lesen\n");
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}