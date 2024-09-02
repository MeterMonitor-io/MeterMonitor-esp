#include <stdio.h>
#include <sys/time.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_sleep.h"
#include "esp_log.h"
#include "driver/rtc_io.h"
#include "nvs.h"

RTC_DATA_ATTR static struct timeval sleep_enter_time;
RTC_DATA_ATTR int boots = 0;

static void deep_sleep_task() {

    struct timeval now;
    gettimeofday(&now, NULL);
    int sleep_time_ms = (now.tv_sec - sleep_enter_time.tv_sec) * 1000 + (now.tv_usec - sleep_enter_time.tv_usec) / 1000;
    boots++;

    switch (esp_sleep_get_wakeup_cause()) {
        case ESP_SLEEP_WAKEUP_TIMER: {
            printf("Wake up from timer. Time spent in deep sleep: %dms\n", sleep_time_ms);
            printf("Bin jetzt zum %i-mal neugestartet!\n", boots);
            break;
        }

        case ESP_SLEEP_WAKEUP_UNDEFINED:
        default:
            printf("Not a deep sleep reset\n");
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS);


    // Isolate GPIO12 pin from external circuits. This is needed for modules
    // which have an external pull-up resistor on GPIO12 (such as ESP32-WROVER)
    // to minimize current consumption.
    rtc_gpio_isolate(GPIO_NUM_12);

    printf("Entering deep sleep\n");

    // get deep sleep enter time
    gettimeofday(&sleep_enter_time, NULL);

    // enter deep sleep
    esp_deep_sleep_start();
}

static void example_deep_sleep_register_rtc_timer_wakeup(void)
{
    const int wakeup_time_sec = 10;
    printf("Enabling timer wakeup, %ds\n", wakeup_time_sec);
    ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(wakeup_time_sec * 1000000));
}

void app_main(void)
{
    /* Enable wakeup from deep sleep by rtc timer */
    example_deep_sleep_register_rtc_timer_wakeup();

    xTaskCreate(deep_sleep_task, "deep_sleep_task", 4096, NULL, 6, NULL);
}