#include "freertos/FreeRTOS.h"
#include "driver/rtc_io.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include "esp_sleep.h"
#include <sys/time.h>
#include "esp_log.h"
#include <stdio.h>
#include "nvs.h"

#include "wifi.h"
#include "mqtt.h"

RTC_DATA_ATTR static struct timeval sleep_enter_time;
static const char *TAG = "Picture-Task";
static struct timeval wake_up_time;
RTC_DATA_ATTR int boots;


static void configure_timer_wakeup() {
    struct timeval now;
    gettimeofday(&now, NULL);
    time_t active_time_ms = (now.tv_sec - wake_up_time.tv_sec) * 1000 + (now.tv_usec - wake_up_time.tv_usec) / 1000;
    ESP_LOGI(TAG, "The device was active for %lld milliseconds", active_time_ms);

    const int default_sleep_time_ms = CONFIG_WATER_METER_SLEEP_TIME * 1000;                                                                //TODO:Change back to 60000 for real minutes(now 1000 for seconds)
    time_t actual_sleep_time_ms = default_sleep_time_ms - active_time_ms;
    ESP_LOGI(TAG, "Theoretically timer should be %d milliseconds", default_sleep_time_ms);
    ESP_LOGI(TAG, "Activating deep sleep Timer for %lld milliseconds", actual_sleep_time_ms);
    ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(actual_sleep_time_ms * 1000));
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
    ESP_LOGI(TAG, "Entering deep sleep");
    esp_deep_sleep_start();
}

static void picture_capture_task() {
    // -----------------------------------------------------------------------------------------------------------------
    // --------------------------------------- TIMEKEEPING AND LOGGING --------------------------------------------------
    // -----------------------------------------------------------------------------------------------------------------
    gettimeofday(&wake_up_time, NULL);

    time_t sleep_time_ms = (wake_up_time.tv_sec - sleep_enter_time.tv_sec) * 1000
                        + (wake_up_time.tv_usec - sleep_enter_time.tv_usec) / 1000;
    switch (esp_sleep_get_wakeup_cause()) {
        case ESP_SLEEP_WAKEUP_TIMER: {
            ESP_LOGI(TAG, "Wake up from timer. Time spent in deep sleep: %lld milliseconds", sleep_time_ms);
            break;
        }
        case ESP_SLEEP_WAKEUP_UNDEFINED:
        default:
            ESP_LOGI(TAG, "Device is booting...");
    }

    // -----------------------------------------------------------------------------------------------------------------
    // ------------------------------------------- INIT WIFI & MQTT ----------------------------------------------------
    // -----------------------------------------------------------------------------------------------------------------
    connect_wifi();
    start_mqtt();

    // -----------------------------------------------------------------------------------------------------------------
    // --------------------------------------------- TAKE PICTURE ------------------------------------------------------
    // -----------------------------------------------------------------------------------------------------------------

    // -----------------------------------------------------------------------------------------------------------------
    // --------------------------------------------- SEND PICTURE ------------------------------------------------------
    // -----------------------------------------------------------------------------------------------------------------
    char sNum[5];
    itoa(boots, sNum, 10);
    publish_message(sNum);
    boots++;

    // -----------------------------------------------------------------------------------------------------------------
    // ------------------------------------------ DE-INIT WIFI & MQTT --------------------------------------------------
    // -----------------------------------------------------------------------------------------------------------------
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