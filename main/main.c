#include "freertos/FreeRTOS.h"
#include "mbedtls/base64.h"
#include "driver/rtc_io.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include "esp_sleep.h"
#include <sys/time.h>
#include <esp_wifi.h>
#include "esp_log.h"
#include <string.h>
#include <stdio.h>
#include "nvs.h"
#include "cJSON.h"

#include "camera.h"
#include "wifi.h"
#include "sntp.h"
#include "mqtt.h"

#define METER_MONITOR_NAME      CONFIG_METER_MONITOR_NAME

RTC_DATA_ATTR static struct timeval sleep_enter_time;
static const char *TAG = "Picture-Task";
static struct timeval wake_up_time;
RTC_DATA_ATTR int bootCount;
int8_t wifiRSSI;


static void configure_timer_wakeup() {
    const int default_sleep_time_ms = CONFIG_METER_MONITOR_SLEEP_TIME * 60000;

    // If it is the first boot, the wake_up_time should not be right due to the time not being synced at boot
    // Meaning: Sleep for the predefined amount of time
    // Else: Try to compensate for the time the ESP has been working to minimize the time deviation
    if (bootCount == 1) {
        ESP_LOGI(TAG, "Activating deep sleep Timer for %d milliseconds", default_sleep_time_ms);
        ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(default_sleep_time_ms * 1000));
    } else {
        struct timeval now;
        gettimeofday(&now, NULL);
        time_t active_time_ms = (now.tv_sec - wake_up_time.tv_sec) * 1000 + (now.tv_usec - wake_up_time.tv_usec) / 1000;
        ESP_LOGI(TAG, "The device was active for %lld milliseconds", active_time_ms);

        time_t actual_sleep_time_ms = default_sleep_time_ms - active_time_ms;
        ESP_LOGI(TAG, "Theoretically timer should be %d milliseconds", default_sleep_time_ms);
        ESP_LOGI(TAG, "Activating deep sleep Timer for %lld milliseconds", actual_sleep_time_ms);
        ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(actual_sleep_time_ms * 1000));
    }
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
    ++bootCount;
    // -----------------------------------------------------------------------------------------------------------------
    // --------------------------------------- TIMEKEEPING AND LOGGING -------------------------------------------------
    // -----------------------------------------------------------------------------------------------------------------
    // Zeitzone auf "Europe/Berlin" (CET/CEST) einstellen (Wird nicht nach DeepSleep gespeichert)
    setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
    tzset();
    gettimeofday(&wake_up_time, NULL);

    time_t sleep_time_ms = (wake_up_time.tv_sec - sleep_enter_time.tv_sec) * 1000
                        + (wake_up_time.tv_usec - sleep_enter_time.tv_usec) / 1000;
    switch (esp_sleep_get_wakeup_cause()) {
        case ESP_SLEEP_WAKEUP_TIMER: {
            ESP_LOGI(TAG, "Wake up from timer. Time spent in deep sleep: %lld milliseconds", sleep_time_ms);
            break;
        }
        default:
            ESP_LOGI(TAG, "Device is booting...");
    }

    // -----------------------------------------------------------------------------------------------------------------
    // ----------------------------------------------- INIT WIFI -------------------------------------------------------
    // -----------------------------------------------------------------------------------------------------------------
    connect_wifi();

    // Hole und Speichere RSSI-Informationen zum verbundenen Access-Point
    wifi_ap_record_t ap_info;
    ESP_ERROR_CHECK(esp_wifi_sta_get_ap_info(&ap_info));
    ESP_LOGI(TAG, "SSID: %s, RSSI: %d dBm", ap_info.ssid, ap_info.rssi);
    wifiRSSI = ap_info.rssi;

    // -----------------------------------------------------------------------------------------------------------------
    // ------------------------------------------- Sync Time IF needed -------------------------------------------------
    // -----------------------------------------------------------------------------------------------------------------
    time_t now;
    struct tm timeInfo;
    time(&now);
    localtime_r(&now, &timeInfo);

    // Is time set? If not, tm_year will be (2024 - 1900).
    if (timeInfo.tm_year < (2024 - 1900)) {
        ESP_LOGI(TAG, "Time is not set yet. Syncing time over NTP.");
        obtain_time();
    }

    // -----------------------------------------------------------------------------------------------------------------
    // ----------------------------------------------- INIT MQTT -------------------------------------------------------
    // -----------------------------------------------------------------------------------------------------------------
    start_mqtt();

    // -----------------------------------------------------------------------------------------------------------------
    // --------------------------------------------- TAKE PICTURE ------------------------------------------------------
    // -----------------------------------------------------------------------------------------------------------------
    ESP_ERROR_CHECK(init_camera());
    camera_fb_t *pic = take_picture();

    // -----------------------------------------------------------------------------------------------------------------
    // --------------------------------------------- SEND PICTURE ------------------------------------------------------
    // -----------------------------------------------------------------------------------------------------------------

    // Base64 Encode the image buffer
    size_t output_len;
    unsigned char *base64_data = malloc(4 * ((pic->len + 2) / 3) + 1);  // Allocate memory for Base64 string
    mbedtls_base64_encode(base64_data, 4 * ((pic->len + 2) / 3) + 1, &output_len, pic->buf, pic->len);

    // Construct Timestamp-String
    struct timeval tv;
    gettimeofday(&tv, NULL);
    time_t nowTime = tv.tv_sec;
    struct tm* nowTm = localtime(&nowTime);
    char timeString[64];
    strftime(timeString, sizeof timeString, "%Y-%m-%dT%H:%M:%S", nowTm);

    // Construct JSON for transport via MQTT
    cJSON *root,*picNode;
    root = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "name", cJSON_CreateString(METER_MONITOR_NAME));
    cJSON_AddNumberToObject(root, "picture_number", bootCount);
    cJSON_AddNumberToObject(root, "WiFi-RSSI", wifiRSSI);
    cJSON_AddItemToObject(root, "picture", picNode=cJSON_CreateObject());
    cJSON_AddStringToObject(picNode, "format", "jpeg");
    cJSON_AddStringToObject(picNode, "timestamp", timeString);
    cJSON_AddNumberToObject(picNode, "width", pic->width);
    cJSON_AddNumberToObject(picNode, "height", pic->height);
    cJSON_AddNumberToObject(picNode, "length", pic->len);
    cJSON_AddStringToObject(picNode, "data", (char *) base64_data);

    // Send message via MQTT
    publish_message(cJSON_Print(root));

    // -----------------------------------------------------------------------------------------------------------------
    // ------------------------------------------ Clean up workspace! --------------------------------------------------
    // -----------------------------------------------------------------------------------------------------------------
    esp_camera_fb_return(pic);
    free(base64_data);

    // -----------------------------------------------------------------------------------------------------------------
    // -------------------------------------- DE-INIT CAMERA & MQTT & WIFI ---------------------------------------------
    // -----------------------------------------------------------------------------------------------------------------
    //ESP_ERROR_CHECK(free_camera());
    stop_mqtt();
    //disconnect_wifi();

    // -----------------------------------------------------------------------------------------------------------------
    // ---------------------------------------------- START SLEEP ------------------------------------------------------
    // -----------------------------------------------------------------------------------------------------------------
    start_deep_sleep();
}


void app_main(void) {
    esp_log_level_set("*", ESP_LOG_WARN);
    esp_log_level_set(TAG, ESP_LOG_INFO);

    // Main task, that is run after every boot/wakeup
    xTaskCreate(picture_capture_task, "picture_capture_task", 4096, NULL, 1, NULL);
}
