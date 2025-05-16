#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_timer.h"
#include "esp_log.h"

#define PRESS_CHANNEL     ADC_CHANNEL_7   // GPIO35
#define ECG_CHANNEL       ADC_CHANNEL_6   // para graficar
#define LED_PIN           GPIO_NUM_2

#define SAMPLE_RATE_HZ      50
#define MAX_PEAKS           300
#define PRESS_THRESHOLD     30
#define CPM_WINDOW_MS       5000  // ventana de 5 segundos para CPM

#define GOOD_INTERVAL_MIN   100  // ms, intervalo mínimo ritmo correcto
#define GOOD_INTERVAL_MAX   120  // ms, intervalo máximo ritmo correcto

static const char *TAG = "PRESS_APP";

// Variables globales
adc_oneshot_unit_handle_t adc1_handle;
int peak_timestamps[MAX_PEAKS];
int peak_count = 0;

// Inicializa ADC
void init_adc() {
    adc_oneshot_unit_init_cfg_t init_cfg = {
        .unit_id = ADC_UNIT_1,
    };
    adc_oneshot_new_unit(&init_cfg, &adc1_handle);

    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = ADC_ATTEN_DB_11,
        .bitwidth = ADC_BITWIDTH_DEFAULT
    };
    adc_oneshot_config_channel(adc1_handle, PRESS_CHANNEL, &chan_cfg);
    adc_oneshot_config_channel(adc1_handle, ECG_CHANNEL, &chan_cfg);
}

// Inicializa LED
void init_led() {
    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
}

// Guarda timestamp de pico detectado
void record_peak(int64_t timestamp_ms) {
    if (peak_count < MAX_PEAKS) {
        peak_timestamps[peak_count++] = timestamp_ms;
    } else {
        for (int i = 1; i < MAX_PEAKS; i++) {
            peak_timestamps[i - 1] = peak_timestamps[i];
        }
        peak_timestamps[MAX_PEAKS - 1] = timestamp_ms;
    }
}

// Calcula CPM en ventana deslizante, ignora zeros
float calculate_cpm() {
    int64_t now_ms = esp_timer_get_time() / 1000;
    int valid_count = 0;

    for (int i = 0; i < peak_count; i++) {
        if (peak_timestamps[i] != 0 && (now_ms - peak_timestamps[i] <= CPM_WINDOW_MS)) {
            valid_count++;
        }
    }

    if (valid_count == 0) return 0.0f;

    return (valid_count * 60000.0f) / CPM_WINDOW_MS;
}

void app_main() {
    init_adc();
    init_led();

    // Inicializar buffer picos a 0
    for (int i = 0; i < MAX_PEAKS; i++) {
        peak_timestamps[i] = 0;
    }
    peak_count = 0;

    int pressure = 0, ecg = 0;
    bool pressure_peak = false;
    int64_t last_peak_time = 0;

    while (1) {
        int64_t now_ms = esp_timer_get_time() / 1000;

        adc_oneshot_read(adc1_handle, PRESS_CHANNEL, &pressure);
        adc_oneshot_read(adc1_handle, ECG_CHANNEL, &ecg);

        // Detección flanco de subida presión
        if (!pressure_peak && pressure < PRESS_THRESHOLD) {
            int64_t interval = now_ms - last_peak_time;
            record_peak(now_ms);
            last_peak_time = now_ms;

            if (interval >= GOOD_INTERVAL_MIN && interval <= GOOD_INTERVAL_MAX) {
                gpio_set_level(LED_PIN, 0);  // LED apagado
                //ESP_LOGI(TAG, "Pulso OK (%lld ms) → LED OFF", interval);
            } else {
                gpio_set_level(LED_PIN, 1);  // LED encendido
                //ESP_LOGW(TAG, "Pulso fuera de ritmo (%lld ms) → LED ON", interval);
            }

            pressure_peak = true;
        }

        if (pressure >= PRESS_THRESHOLD) {
            pressure_peak = false;
        }

        float cpm = calculate_cpm();

        int press_binary = (pressure < PRESS_THRESHOLD) ? 1 : 0;
        printf("ECG:%d,PRESS:%d,CPM:%.2f\n", ecg, press_binary, cpm);


        vTaskDelay(pdMS_TO_TICKS(1000 / SAMPLE_RATE_HZ));
    }
}
