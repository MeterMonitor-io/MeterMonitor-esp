#include "freertos/FreeRTOS.h"
#include "mbedtls/base64.h"
#include "driver/rtc_io.h"
#include "freertos/task.h"
#include <sys/unistd.h>
#include "esp_sleep.h"
#include <sys/time.h>
#include <esp_wifi.h>
#include "esp_log.h"
#include <string.h>
#include "cJSON.h"
#include "nvs.h"

#include "configurations.h"
#include "esp_ws28xx.h"
#include "nvs_helper.h"
#include "sd_card.h"
#include "camera.h"
#include "wifi.h"
#include "sntp.h"
#include "mqtt.h"

RTC_DATA_ATTR static struct timeval sleep_enter_time;
static const char *bootCountKey = "bootCount";
static const char *TAG = "[Picture-Task]";
static struct timeval wake_up_time;
bool timeSyncing = false;
bool isCleanBoot = false;
uint32_t bootCount;
int8_t wifiRSSI;


static void configure_timer_wakeup() {
    const long long default_sleep_time_ms = config.sleep_time_min * 60000;

    // If it is the first boot, the wake_up_time should not be right due to the time not being synced at boot
    // Meaning: Sleep for the predefined amount of time
    // Else: Try to compensate for the time the ESP has been working to minimize the time deviation
    if (isCleanBoot) {
        ESP_LOGI(TAG, "Activating deep sleep timer for %d minutes", config.sleep_time_min);
        ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(default_sleep_time_ms * 1000));
    } else {
        struct timeval now;
        gettimeofday(&now, NULL);
        const time_t active_time_ms = (now.tv_sec - wake_up_time.tv_sec) * 1000 +
                                      (now.tv_usec - wake_up_time.tv_usec) / 1000;
        ESP_LOGI(TAG, "The device was active for %lld milliseconds", active_time_ms);

        const time_t actual_sleep_time_ms = default_sleep_time_ms - active_time_ms;
        ESP_LOGI(TAG, "Theoretically timer should be %d minutes", config.sleep_time_min);
        ESP_LOGI(TAG, "Activating deep sleep Timer for %lld milliseconds", actual_sleep_time_ms);
        ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(actual_sleep_time_ms * 1000));
    }
}


// Send 'Done'-Signal to TPL5110 Low-power Timer to shut off power externally
static void send_done_signal() {
    ESP_ERROR_CHECK(gpio_set_direction(config.done_gpio, GPIO_MODE_OUTPUT));
    ESP_LOGI(TAG, "Sending 'Done' signal...");
    for (int i = 1; i <= 3; ++i) {
        ESP_LOGI(TAG, "Sending signal #%i on GPIO%d", i, config.done_gpio);
        fflush(stdout);
        fsync(fileno(stdout));
        ESP_ERROR_CHECK(gpio_set_level(config.done_gpio, 1));
        vTaskDelay(pdMS_TO_TICKS(200));
        ESP_ERROR_CHECK(gpio_set_level(config.done_gpio, 0));
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    ESP_LOGW(TAG, "Power could not be cut successfully, continuing to go into deep sleep!");
}


static void start_deep_sleep() {
    // Isolate GPIO12 pin from external circuits.
    // This is necessary for modules which have an external pull-up resistor on GPIO12 to minimize current consumption.
    rtc_gpio_isolate(GPIO_NUM_12);

    // Enable wakeup from deep sleep by rtc timer
    configure_timer_wakeup();

    // Save the time the deep sleep is entered
    gettimeofday(&sleep_enter_time, NULL);

    // Enter deep sleep
    ESP_LOGI(TAG, "Entering deep sleep...");
    esp_deep_sleep_start();
}


static void picture_capture_task() {
    // -----------------------------------------------------------------------------------------------------------------
    // --------------------------------------------- INIT CONFIG -------------------------------------------------------
    // -----------------------------------------------------------------------------------------------------------------
    init_config();

    // -----------------------------------------------------------------------------------------------------------------
    // -------------------------------------- READ CUSTOM CONFIG FROM SD-CARD ------------------------------------------
    // -----------------------------------------------------------------------------------------------------------------
    if (mount_sd_card()) {
        if (!import_settings_from_file()) {
            ESP_LOGW(TAG, "Could not import settings from SD-Card. Skipping file import");
        }
        unmount_sd_card();
    } else {
        ESP_LOGW(TAG, "Could not initialise SD-Card. Skipping file import");
    }

    // -----------------------------------------------------------------------------------------------------------------
    // ----------------------------------------------- INIT NVS --------------------------------------------------------
    // -----------------------------------------------------------------------------------------------------------------
    if (!init_nvs()) return;

    // -----------------------------------------------------------------------------------------------------------------
    // -------------------------------------- GET BOOT COUNT + INCREMENT -----------------------------------------------
    // -----------------------------------------------------------------------------------------------------------------
    read_value(bootCountKey, &bootCount);
    ++bootCount;
    ESP_LOGI(TAG, "Boot count: %"PRIu32, bootCount);

    // -----------------------------------------------------------------------------------------------------------------
    // --------------------------------------- TIMEKEEPING AND LOGGING -------------------------------------------------
    // -----------------------------------------------------------------------------------------------------------------
    // Zeitzone auf "Europe/Berlin" (CET/CEST) einstellen (Wird nicht nach DeepSleep gespeichert)
    setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
    tzset();
    gettimeofday(&wake_up_time, NULL);

    switch (esp_sleep_get_wakeup_cause()) {
        case ESP_SLEEP_WAKEUP_TIMER: {
            const time_t sleep_time_ms = (wake_up_time.tv_sec - sleep_enter_time.tv_sec) * 1000
                                         + (wake_up_time.tv_usec - sleep_enter_time.tv_usec) / 1000;
            ESP_LOGI(TAG, "Wake up from timer. Time spent in deep sleep: %lld milliseconds", sleep_time_ms);
            break;
        }
        default:
            isCleanBoot = true;
            ESP_LOGI(TAG, "Device is booting...");
    }

    // -----------------------------------------------------------------------------------------------------------------
    // ----------------------------------------------- INIT WIFI -------------------------------------------------------
    // -----------------------------------------------------------------------------------------------------------------
    connect_wifi();

    // Get and save RSSI information for the connected access point
    wifi_ap_record_t ap_info;
    ESP_ERROR_CHECK(esp_wifi_sta_get_ap_info(&ap_info));
    ESP_LOGI(TAG, "SSID: %s, RSSI: %d dBm", (char *) ap_info.ssid, ap_info.rssi);
    wifiRSSI = ap_info.rssi;

    // -----------------------------------------------------------------------------------------------------------------
    // ------------------------------------------- Sync Time IF needed -------------------------------------------------
    // -----------------------------------------------------------------------------------------------------------------
    time_t now;
    struct tm timeInfo;
    time(&now);
    localtime_r(&now, &timeInfo);

    // Is the time set? If not, tm_year will be (2024-1900).
    if (timeInfo.tm_year < (2024 - 1900)) {
        ESP_LOGI(TAG, "Time is not set yet. Syncing time over SNTP.");
        timeSyncing = true;
    } else if (config.sntp_time_sync_always) {
        ESP_LOGI(TAG, "Time is set, but may be out of sync. Syncing time at every boot.");
        timeSyncing = true;
    } else {
        ESP_LOGI(TAG, "Time is set and will not be synced again.");
    }

    if (timeSyncing) {
        start_obtaining_time();
    }

    // -----------------------------------------------------------------------------------------------------------------
    // ----------------------------------------------- INIT MQTT -------------------------------------------------------
    // -----------------------------------------------------------------------------------------------------------------
    start_mqtt();

    // -----------------------------------------------------------------------------------------------------------------
    // --------------------------------------------- TAKE PICTURE ------------------------------------------------------
    // -----------------------------------------------------------------------------------------------------------------
    // If WS2812B LEDs are to be used, turn them on (and free) before init_camera() because of SPI DMA collisions
    if (config.led_strip) {
        CRGB* ws2812_buffer;
        ESP_ERROR_CHECK(ws28xx_init(config.led_strip_gpio, WS2812B, config.led_strip_count, &ws2812_buffer));
        ws28xx_fill_all((CRGB){.r=config.led_r, .g=config.led_g, .b=config.led_b});
        ESP_ERROR_CHECK(ws28xx_update());
        ESP_LOGI(TAG, "LED-Strip turned on successfully");
        ESP_ERROR_CHECK(ws28xx_free());
    }

    ESP_ERROR_CHECK(init_camera());
    camera_fb_t *pic = take_picture();

    // If WS2812B LEDs are used, turn them off after the picture has been taken
    if (config.led_strip) {
        CRGB* ws2812_buffer;
        ESP_ERROR_CHECK(ws28xx_init(config.led_strip_gpio, WS2812B, config.led_strip_count, &ws2812_buffer));
        ws28xx_fill_all((CRGB){.r=0, .g=0, .b=0});
        ESP_ERROR_CHECK(ws28xx_update());
        ESP_LOGI(TAG, "LED-Strip turned off successfully");
        ESP_ERROR_CHECK(ws28xx_free());
    }

    // -----------------------------------------------------------------------------------------------------------------
    // --------------------------------------------- SEND PICTURE ------------------------------------------------------
    // -----------------------------------------------------------------------------------------------------------------
    const size_t base64_malloc_length = 4 * ((pic->len + 2) / 3) + 1;
    const size_t largest_space_av = heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT);
    if (base64_malloc_length > largest_space_av) {
        ESP_LOGW(TAG, "Largest free block in memory: %d bytes", largest_space_av);
        ESP_LOGW(TAG, "Space needed for Base64-Encoding: %d bytes", base64_malloc_length);
        ESP_LOGE(TAG, "Not enough free memory to encode the image. Try lowering the resolution!");
        return;
    }

    // Base64 Encode the image buffer
    size_t output_len;
    unsigned char *base64_data = malloc(base64_malloc_length); // Allocate memory for Base64 string
    mbedtls_base64_encode(base64_data, base64_malloc_length, &output_len, pic->buf, pic->len);

    // Cache pic variables before freeing
    const size_t pic_width = pic->width;
    const size_t pic_height = pic->height;
    const size_t pic_len = pic->len;

    // Free up space for JSON construction (without there is no continuous free block in memory)
    esp_camera_fb_return(pic);
    ESP_ERROR_CHECK(free_camera());

    // Construct Timestamp-String
    if (timeSyncing) wait_for_time_sync();
    struct timeval tv;
    gettimeofday(&tv, NULL);
    const time_t nowTime = tv.tv_sec;
    const struct tm *nowTm = localtime(&nowTime);
    char timeString[64];
    strftime(timeString, sizeof timeString, "%Y-%m-%dT%H:%M:%S", nowTm);

    // Construct JSON for transport via MQTT
    cJSON *root = cJSON_CreateObject();
    cJSON *picNode = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "name", cJSON_CreateString(config.meter_monitor_name));
    cJSON_AddNumberToObject(root, "picture_number", (double)bootCount);
    cJSON_AddNumberToObject(root, "WiFi-RSSI", wifiRSSI);
    cJSON_AddItemToObject(root, "picture", picNode);
    cJSON_AddStringToObject(picNode, "format", "jpeg");
    cJSON_AddStringToObject(picNode, "timestamp", timeString);
    cJSON_AddNumberToObject(picNode, "width", pic_width);
    cJSON_AddNumberToObject(picNode, "height", pic_height);
    cJSON_AddNumberToObject(picNode, "length", pic_len);
    cJSON_AddStringToObject(picNode, "data", (char *) base64_data);

    // Send the message via MQTT
    char *msg_string = cJSON_Print(root);
    publish_message(msg_string);

    // -----------------------------------------------------------------------------------------------------------------
    // ------------------------------------------ Clean up workspace! --------------------------------------------------
    // -----------------------------------------------------------------------------------------------------------------
    cJSON_Delete(root);
    cJSON_free(msg_string);
    free(base64_data);

    // -----------------------------------------------------------------------------------------------------------------
    // ------------------------------------------ DE-INIT MQTT & WIFI --------------------------------------------------
    // -----------------------------------------------------------------------------------------------------------------
    stop_mqtt();
    //disconnect_wifi();

    // -----------------------------------------------------------------------------------------------------------------
    // ------------------------------------------- STORE BOOT COUNT ----------------------------------------------------
    // -----------------------------------------------------------------------------------------------------------------
    if (!write_value(bootCountKey, bootCount)) return;

    // -----------------------------------------------------------------------------------------------------------------
    // ----------------------------------------------- CLOSE NVS -------------------------------------------------------
    // -----------------------------------------------------------------------------------------------------------------
    close_nvs();

    // -----------------------------------------------------------------------------------------------------------------
    // ----------------------------------------- Disable power supply --------------------------------------------------
    // -----------------------------------------------------------------------------------------------------------------
    if (config.send_done) {
        send_done_signal();
    } else {
        ESP_LOGW(TAG, "Done signal is disabled, continuing to go into deep sleep!");
    }

    // After this, the power should be cut off externally (if configured), but if not, go into deepsleep.
    // -----------------------------------------------------------------------------------------------------------------
    // ---------------------------------------------- START SLEEP ------------------------------------------------------
    // -----------------------------------------------------------------------------------------------------------------
    start_deep_sleep();
}


void app_main(void) {
    esp_log_level_set("*", ESP_LOG_WARN);
    esp_log_level_set(TAG, ESP_LOG_INFO);

    // Main task that is run after every boot/wakeup
    xTaskCreate(picture_capture_task, "picture_capture_task", 4096, NULL, 1, NULL);
}
