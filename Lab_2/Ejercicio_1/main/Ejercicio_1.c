#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"

#define LED_GPIO GPIO_NUM_33
#define BUTTON_GPIO GPIO_NUM_0

volatile int contador = 0;
volatile int hello_counter = 0;

void hello_task(void *pvParameter)
{
    while (1) {
        printf("Hello World\n");
        hello_counter++;

        if (hello_counter >= 10) {
            hello_counter = 0;
            contador = 0;  // Reinicia contador
        }

        vTaskDelay(pdMS_TO_TICKS(2000));  // Espera 2 segundos
    }
}

void blink_led_task(void *pvParameter)
{
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);

    while (1) {
        gpio_set_level(LED_GPIO, 1);
        vTaskDelay(pdMS_TO_TICKS(1000 / (contador + 1)));  // A mayor contador, más rápido
        gpio_set_level(LED_GPIO, 0);
        vTaskDelay(pdMS_TO_TICKS(1000 / (contador + 1)));
    }
}

void button_task(void *pvParameter)
{
    gpio_set_direction(BUTTON_GPIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BUTTON_GPIO, GPIO_PULLUP_ONLY);  // Evita ruido

    int last_state = 1;

    while (1) {
        int current_state = gpio_get_level(BUTTON_GPIO);

        if (last_state == 1 && current_state == 0) {  // Detecta flanco descendente
            contador++;
            printf("Botón presionado. Contador = %d\n", contador);
            vTaskDelay(pdMS_TO_TICKS(200));  // debounce
        }

        last_state = current_state;
        vTaskDelay(pdMS_TO_TICKS(10));  // Escaneo rápido
    }
}

void app_main(void)
{
    xTaskCreate(&hello_task, "hello_task", 2048, NULL, 1, NULL);
    xTaskCreate(&blink_led_task, "blink_task", 2048, NULL, 1, NULL);
    xTaskCreate(&button_task, "button_task", 2048, NULL, 1, NULL);
}
