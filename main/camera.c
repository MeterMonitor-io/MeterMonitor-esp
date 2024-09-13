#include "camera.h"

#include <esp_system.h>
#include "esp_camera.h"
#include "esp_ws28xx.h"
#include <rom/gpio.h>
#include <esp_log.h>

#define LED_NUM             CONFIG_METER_MONITOR_LED_STRIP_LED_COUNT
#define LED_GPIO            CONFIG_METER_MONITOR_LED_STRIP_GPIO
#define LED_STRIP_R         CONFIG_METER_MONITOR_LED_STRIP_R
#define LED_STRIP_G         CONFIG_METER_MONITOR_LED_STRIP_G
#define LED_STRIP_B         CONFIG_METER_MONITOR_LED_STRIP_B
#define FLASH_LED_GPIO      CONFIG_METER_MONITOR_FLASH_GPIO

#ifdef CONFIG_METER_MONITOR_BOARD_ESP32_CAM
// AiThinker ESP32-Cam PIN Map
#define CAM_PIN_PWDN        32
#define CAM_PIN_RESET       (-1) //software reset will be performed
#define CAM_PIN_XCLK        0
#define CAM_PIN_SIOD        26
#define CAM_PIN_SIOC        27
#define CAM_PIN_D7          35
#define CAM_PIN_D6          34
#define CAM_PIN_D5          39
#define CAM_PIN_D4          36
#define CAM_PIN_D3          21
#define CAM_PIN_D2          19
#define CAM_PIN_D1          18
#define CAM_PIN_D0          5
#define CAM_PIN_VSYNC       25
#define CAM_PIN_HREF        23
#define CAM_PIN_PCLK        22
#endif

#ifdef CONFIG_METER_MONITOR_BOARD_SEEED_XIAO_ESP32S3_SENSE
// Seeed Studio XIAO ESP32S3 Sense PIN Map
#define CAM_PIN_PWDN        (-1)
#define CAM_PIN_RESET       (-1)
#define CAM_PIN_XCLK        10
#define CAM_PIN_SIOD        40
#define CAM_PIN_SIOC        39
#define CAM_PIN_D7          48
#define CAM_PIN_D6          11
#define CAM_PIN_D5          12
#define CAM_PIN_D4          14
#define CAM_PIN_D3          16
#define CAM_PIN_D2          18
#define CAM_PIN_D1          17
#define CAM_PIN_D0          15
#define CAM_PIN_VSYNC       38
#define CAM_PIN_HREF        47
#define CAM_PIN_PCLK        13
#endif

static const char *TAG = "Camera-Controller";
static const uint8_t CAM_INIT_MAX_TRIES = 10;
CRGB* ws2812_buffer;

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

    .xclk_freq_hz = 10000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,
    .pixel_format = PIXFORMAT_JPEG,

    .jpeg_quality = 12,
    .fb_count = 1,
    .fb_location = CAMERA_FB_IN_PSRAM,
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
#if CONFIG_MONITOR_CAMERA_CAMERA_FRAME_SIZE_QQVGA
    .frame_size = FRAMESIZE_QQVGA,
#elif CONFIG_MONITOR_CAMERA_CAMERA_FRAME_SIZE_QVGA
    .frame_size = FRAMESIZE_QVGA,
#elif CONFIG_METER_MONITOR_CAMERA_FRAME_SIZE_VGA
    .frame_size = FRAMESIZE_VGA,
#elif CONFIG_METER_MONITOR_CAMERA_FRAME_SIZE_SVGA
    .frame_size = FRAMESIZE_SVGA,
#elif CONFIG_METER_MONITOR_CAMERA_FRAME_SIZE_XGA
    .frame_size = FRAMESIZE_XGA,
#elif CONFIG_METER_MONITOR_CAMERA_FRAME_SIZE_SXGA
    .frame_size = FRAMESIZE_SXGA,
#elif CONFIG_METER_MONITOR_CAMERA_FRAME_SIZE_UXGA
    .frame_size = FRAMESIZE_UXGA,
#endif
};


void set_camera_parameters() {
    sensor_t *s = esp_camera_sensor_get();
    if (s != NULL) {
        s->set_aec2(s, 1);  // AEC (Automatic Exposure Control) Modus 2 aktivieren
        s->set_awb_gain(s, 1);  // Automatischen Weißabgleich aktivieren
        ESP_LOGI(TAG, "Kamerasensor-Parameter erfolgreich gesetzt");
    } else {
        ESP_LOGE(TAG, "Kamerasensor konnte nicht geladen werden");
    }
}

esp_err_t init_camera(void) {
    esp_err_t err;
    for (int i = 1; i <= CAM_INIT_MAX_TRIES; ++i) {
        err = esp_camera_init(&camera_config);

        if(err == ESP_OK) break;
        else ESP_LOGW(TAG, "Esp32 Camera Init failed on try (%d/%d)", i, CAM_INIT_MAX_TRIES);
    }

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Camera Init Failed");
        return err;
    }
    set_camera_parameters();
    return ESP_OK;
}

camera_fb_t* take_picture(void) {
#ifdef CONFIG_METER_MONITOR_FLASH
    gpio_pad_select_gpio(FLASH_LED_GPIO);
    ESP_ERROR_CHECK(gpio_set_direction(FLASH_LED_GPIO, GPIO_MODE_OUTPUT));
    ESP_ERROR_CHECK(gpio_set_level(FLASH_LED_GPIO, 1));
    ESP_LOGI(TAG, "Flash turned on successfully");
#endif
#ifdef CONFIG_METER_MONITOR_LED_STRIP
    ESP_ERROR_CHECK(ws28xx_init(LED_GPIO, WS2812B, LED_NUM, &ws2812_buffer));
    ws28xx_fill_all((CRGB){.r=LED_STRIP_R, .g=LED_STRIP_G, .b=LED_STRIP_B});
    ESP_ERROR_CHECK(ws28xx_update());
    ESP_LOGI(TAG, "LED-Strip turned on successfully");
#endif

    ESP_LOGI(TAG, "Taking picture...");
    camera_fb_t *picture = esp_camera_fb_get();

#ifdef CONFIG_METER_MONITOR_FLASH
    ESP_ERROR_CHECK(gpio_set_level(FLASH_LED_GPIO, 0));
    ESP_LOGI(TAG, "Flash turned off successfully");
#endif
#ifdef CONFIG_METER_MONITOR_LED_STRIP
    ws28xx_fill_all((CRGB){.r=0, .g=0, .b=0});
    ESP_ERROR_CHECK(ws28xx_update());
    ESP_LOGI(TAG, "LED-Strip turned off successfully");
#endif
    return picture;
}
