/**
 * This example takes a picture every 5s and print its size on serial monitor.
 */

// =============================== SETUP ======================================

// 1. Board setup (Uncomment):
// #define BOARD_WROVER_KIT
// #define BOARD_ESP32CAM_AITHINKER
// #define BOARD_ESP32S3_WROOM

/**
 * 2. Kconfig setup
 *
 * If you have a Kconfig file, copy the content from
 *  https://github.com/espressif/esp32-camera/blob/master/Kconfig into it.
 * In case you haven't, copy and paste this Kconfig file inside the src directory.
 * This Kconfig file has definitions that allows more control over the camera and
 * how it will be initialized.
 */

/**
 * 3. Enable PSRAM on sdkconfig:
 *
 * CONFIG_ESP32_SPIRAM_SUPPORT=y
 *
 * More info on
 * https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/kconfig.html#config-esp32-spiram-support
 */

// ================================ CODE ======================================

#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "esp_dsp.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


#include <stdlib.h>
//#include "esp_heap_caps.h"
#include "esp_spiffs.h"




float sobel_x[9] = {-1, 0, 1,
    -2, 0, 2,
    -1, 0, 1};

float sobel_y[9] = {-1, -2, -1,
     0,  0,  0,
     1,  2,  1};



// support IDF 5.x
#ifndef portTICK_RATE_MS
#define portTICK_RATE_MS portTICK_PERIOD_MS
#endif

#include "esp_camera.h"
#define BOARD_ESP32CAM_AITHINKER

#define BOARD_WROVER_KIT 1

// WROVER-KIT PIN Map
#ifdef BOARD_WROVER_KIT

#define CAM_PIN_PWDN -1  //power down is not used
#define CAM_PIN_RESET -1 //software reset will be performed
#define CAM_PIN_XCLK 21
#define CAM_PIN_SIOD 26
#define CAM_PIN_SIOC 27

#define CAM_PIN_D7 35
#define CAM_PIN_D6 34
#define CAM_PIN_D5 39
#define CAM_PIN_D4 36
#define CAM_PIN_D3 19
#define CAM_PIN_D2 18
#define CAM_PIN_D1 5
#define CAM_PIN_D0 4
#define CAM_PIN_VSYNC 25
#define CAM_PIN_HREF 23
#define CAM_PIN_PCLK 22

#endif

// ESP32Cam (AiThinker) PIN Map
#ifdef BOARD_ESP32CAM_AITHINKER

#define CAM_PIN_PWDN 32
#define CAM_PIN_RESET -1 //software reset will be performed
#define CAM_PIN_XCLK 0
#define CAM_PIN_SIOD 26
#define CAM_PIN_SIOC 27

#define CAM_PIN_D7 35
#define CAM_PIN_D6 34
#define CAM_PIN_D5 39
#define CAM_PIN_D4 36
#define CAM_PIN_D3 21
#define CAM_PIN_D2 19
#define CAM_PIN_D1 18
#define CAM_PIN_D0 5
#define CAM_PIN_VSYNC 25
#define CAM_PIN_HREF 23
#define CAM_PIN_PCLK 22

#endif
// ESP32S3 (WROOM) PIN Map
#ifdef BOARD_ESP32S3_WROOM
#define CAM_PIN_PWDN 38
#define CAM_PIN_RESET -1   //software reset will be performed
#define CAM_PIN_VSYNC 6
#define CAM_PIN_HREF 7
#define CAM_PIN_PCLK 13
#define CAM_PIN_XCLK 15
#define CAM_PIN_SIOD 4
#define CAM_PIN_SIOC 5
#define CAM_PIN_D0 11
#define CAM_PIN_D1 9
#define CAM_PIN_D2 8
#define CAM_PIN_D3 10
#define CAM_PIN_D4 12
#define CAM_PIN_D5 18
#define CAM_PIN_D6 17
#define CAM_PIN_D7 16
#endif
static const char *TAG = "example:take_picture";

#if ESP_CAMERA_SUPPORTED
static camera_config_t camera_config = {
    .pin_pwdn = CAM_PIN_PWDN,
    .pin_reset = CAM_PIN_RESET,
    .pin_xclk = CAM_PIN_XCLK,
    .pin_sccb_sda = CAM_PIN_SIOD,
    .pin_sccb_scl = CAM_PIN_SIOC,

    .pin_d7 = CAM_PIN_D7,
    .pin_d6 = CAM_PIN_D6,
    .pin_d5 = CAM_PIN_D5,
    .pin_d4 = CAM_PIN_D4,
    .pin_d3 = CAM_PIN_D3,
    .pin_d2 = CAM_PIN_D2,
    .pin_d1 = CAM_PIN_D1,
    .pin_d0 = CAM_PIN_D0,
    .pin_vsync = CAM_PIN_VSYNC,
    .pin_href = CAM_PIN_HREF,
    .pin_pclk = CAM_PIN_PCLK,

    //XCLK 20MHz or 10MHz for OV2640 double FPS (Experimental)
    .xclk_freq_hz = 20000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_GRAYSCALE, //YUV422,GRAYSCALE,RGB565,JPEG
    .frame_size = FRAMESIZE_96X96,    //QQVGA-UXGA, For ESP32, do not use sizes above QVGA when not JPEG. The performance of the ESP32-S series has improved a lot, but JPEG mode always gives better frame rates.

    .jpeg_quality = 12, //0-63, for OV series camera sensors, lower number means higher quality
    .fb_count = 1,       //When jpeg mode is used, if fb_count more than one, the driver will work in continuous mode.
    .fb_location = CAMERA_FB_IN_DRAM,
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
};

static esp_err_t init_camera(void)
{
    //initialize the camera
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Camera Init Failed");
        return err;
    }

    return ESP_OK;
}
#endif

void equalize_histogram(uint8_t *image, size_t length) {
    int histogram[256] = {0};
    float cdf[256] = {0};
    int cdf_min = -1;

    // 1. Calcular histograma
    for (int i = 0; i < length; i++) {
        histogram[image[i]]++;
    }

    // 2. Calcular CDF
    int total = 0;
    for (int i = 0; i < 256; i++) {
        total += histogram[i];
        cdf[i] = total;
        if (cdf_min == -1 && cdf[i] > 0) {
            cdf_min = cdf[i];
        }
    }

    // 3. Normalizar CDF y aplicar transformación
    for (int i = 0; i < 256; i++) {
        cdf[i] = ((cdf[i] - cdf_min) / (length - cdf_min)) * 255.0f;
        if (cdf[i] < 0) cdf[i] = 0;
        if (cdf[i] > 255) cdf[i] = 255;
    }

    for (int i = 0; i < length; i++) {
        image[i] = (uint8_t)cdf[image[i]];
    }
}

void apply_sobel_filter(uint8_t *input, uint8_t *output, int width, int height) {
    for (int y = 1; y < height - 1; y++) {
        for (int x = 1; x < width - 1; x++) {
            int gx = 0, gy = 0;

            for (int ky = -1; ky <= 1; ky++) {
                for (int kx = -1; kx <= 1; kx++) {
                    int pixel = input[(y + ky) * width + (x + kx)];
                    gx += pixel * sobel_x[(ky + 1) * 3 + (kx + 1)];
                    gy += pixel * sobel_y[(ky + 1) * 3 + (kx + 1)];
                }
            }

            int magnitude = abs(gx) + abs(gy);
            if (magnitude > 255) magnitude = 255;
            output[y * width + x] = (uint8_t)magnitude;
        }
    }
}

void app_main(void)
{
#if ESP_CAMERA_SUPPORTED
    if(ESP_OK != init_camera()) {
        return;
    }

    size_t l = 0;
    bool si = true;

    while (true)
    {
        printf("\nSPIFFS system initiation:\n");
        esp_vfs_spiffs_conf_t conf = {
            .base_path = "/spiffs",
            .partition_label = NULL,
            .max_files = 20,
            .format_if_mount_failed = true};
        esp_vfs_spiffs_register(&conf);
        size_t total = 0, used = 0;
        esp_spiffs_info(conf.partition_label, &total, &used);
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
        ESP_LOGI(TAG, "Taking picture...");
        camera_fb_t *pic = esp_camera_fb_get();
        // para el ej 4 se comenta la siguiente linea
        equalize_histogram(pic->buf, pic->len);
        apply_sobel_filter(pic->buf);

        // Crear buffer para filtro sobel
        uint8_t *sobel_out = malloc(pic->len);
        if (!sobel_out) {
            ESP_LOGE(TAG, "No se pudo asignar memoria para filtro Sobel");
            esp_camera_fb_return(pic);
            continue;
        }

        apply_sobel_filter(pic->buf, sobel_out, pic->width,pic->height);

        // use pic->buf to access the image
        // Write and read file
        printf("\nWrite and read file:\n");
        // Create a file.
        ESP_LOGI(TAG, "Opening file");
        char path[22];
        sprintf(path, "/spiffs/img_%02d.txt", l);
        if (!si){
            remove(path);
        }
        FILE *f = fopen(path, "w");
        

        for (int i = 0; i < pic->len; i++)
        {
            fprintf(f,"0x%02X, ",sobel_out[i]);

            if ((i+1)% 16 == 0)
            {
                fprintf(f,"\n");
            }
                }
                fprintf(f,"\n");
                free(sobel_out);

                fclose(f);
                ESP_LOGI(TAG, "File written");
                ESP_LOGI(TAG, "Picture taken! Its size was: %zu bytes", pic->len);

                printf("\nSPIFFS system unmount:\n");
                esp_vfs_spiffs_unregister(conf.partition_label);
                ESP_LOGI(TAG, "SPIFFS unmounted");
                esp_camera_fb_return(pic);
                
                l++;
                if (l >= 20)
                {
                    l = 0;
                    si = false;
                }

    }
        
    
#else
    ESP_LOGE(TAG, "Camera support is not available for this chip");
    return;
#endif
}
