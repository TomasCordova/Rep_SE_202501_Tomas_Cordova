#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define WIDTH 96
#define HEIGHT 96

// Archivo embebido como binario
extern const uint8_t image_txt_start[] asm("_binary_image_txt_start");
extern const uint8_t image_txt_end[]   asm("_binary_image_txt_end");

uint8_t image[HEIGHT][WIDTH];

int8_t Gx[3][3] = {
    {-1, 0, 1},
    {-2, 0, 2},
    {-1, 0, 1}
};

int8_t Gy[3][3] = {
    {-1, -2, -1},
     {0,  0,  0},
     {1,  2,  1}
};

void cargar_imagen_desde_txt() {
    const char* ptr = (const char*) image_txt_start;
    char buffer[10];
    int i = 0, j = 0, k = 0;

    while (ptr < (const char*) image_txt_end && i < HEIGHT) {
        if (*ptr >= '0' && *ptr <= '9') {
            buffer[k++] = *ptr;
        } else {
            if (k > 0) {
                buffer[k] = '\0';
                int val = atoi(buffer);
                image[i][j++] = (uint8_t) val;
                k = 0;
                if (j == WIDTH) {
                    j = 0;
                    i++;
                }
            }
        }
        ptr++;
    }

    // Último número si el archivo no termina en separador
    if (k > 0 && i < HEIGHT && j < WIDTH) {
        buffer[k] = '\0';
        int val = atoi(buffer);
        image[i][j++] = (uint8_t) val;
    }

    printf("Cargada imagen:\n");
for (int i = 0; i < HEIGHT; i++) {
    printf("Fila %d: ", i);
    for (int j = 0; j < WIDTH; j++) {
        printf("%d ", image[i][j]);
    }
    printf("\n");
}
}

void apply_sobel(uint8_t input[HEIGHT][WIDTH], uint8_t output[HEIGHT][WIDTH]) {
    for (int i = 1; i < HEIGHT - 1; i++) {
        for (int j = 1; j < WIDTH - 1; j++) {
            int gx = 0, gy = 0;

            for (int x = -1; x <= 1; x++) {
                for (int y = -1; y <= 1; y++) {
                    int pixel = input[i + x][j + y];
                    gx += pixel * Gx[x + 1][y + 1];
                    gy += pixel * Gy[x + 1][y + 1];
                }
            }

            int mag = (int) sqrtf((float)(gx * gx + gy * gy));
            if (mag > 255) mag = 255;
            if (mag < 0) mag = 0;
            output[i][j] = (uint8_t) mag;
        }
    }
}

void app_main(void) {
    cargar_imagen_desde_txt();

    static uint8_t sobel[HEIGHT][WIDTH] = {0};
    apply_sobel(image, sobel);

    // Imprimir como matriz para cargar en Colab
    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) {
            printf("%d ", sobel[i][j]);
        }
        printf("\n");
    }
}
