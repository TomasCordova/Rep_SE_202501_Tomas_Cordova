#include <stdio.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_system.h"

#define X 1000  // puedes cambiar esto para distintas iteraciones

int var1 = 233;
int var2 = 128;

void app_main(void)
{
    int result_0 = 0, result_1 = 0, result_2 = 0, result_3 = 0;
    float result_4 = 0.0;
    volatile int dummy_int = 0;
    volatile float dummy_float = 0.0;

    int64_t start_time = esp_timer_get_time();
    uint32_t start_cycles = esp_cpu_get_cycle_count();

    for (int i = 0; i < X; i++) {
        // Descomenta solo una línea para medirla:

        result_0 = var1 + var2;
        dummy_int += result_0;

        result_1 = var1 + 10;
        dummy_int += result_1;

        result_2 = var1 % var2;
        dummy_int += result_2;

        result_3 = var1 * var2;
        dummy_int += result_3;

        result_4 = (float)var1 / (float)var2;
        dummy_float += result_4;
    }

    int64_t end_time = esp_timer_get_time();
    uint32_t end_cycles = esp_cpu_get_cycle_count();

    int64_t elapsed_time_us = end_time - start_time;
    uint32_t elapsed_cycles = end_cycles - start_cycles;

    // CPI estimado: 1 instrucción por iteración activa
    float total_instructions = X * 5.0;
    float CPI = elapsed_cycles / total_instructions;

    // Mostrar resultados
    printf("X: %d\n", X);
    printf("Tiempo total (us): %" PRId64 "\n", elapsed_time_us);
    printf("Ciclos totales: %lu\n", elapsed_cycles);
    printf("CPI estimado: %f\n", CPI);

    if (dummy_int == -1) printf("Dummy int: %d\n", dummy_int);
    if (dummy_float == -1.0) printf("Dummy float: %f\n", dummy_float);
}
