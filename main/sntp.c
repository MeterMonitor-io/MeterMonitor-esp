#include "sntp.h"

#include "configurations.h"

#include "esp_netif_sntp.h"
#include "esp_log.h"
#include <time.h>

#define SNTP_TIME_SYNCED            BIT0
#define SECONDS_TO_WAIT_FOR_TIME    10
static EventGroupHandle_t sntp_event_group;
static const char *TAG = "[SNTP]";


void time_sync_notification_cb(__attribute__((unused)) struct timeval *tv) {
    ESP_LOGI(TAG, "Time has been synced successfully via the SNTP-Server.");
    xEventGroupSetBits(sntp_event_group, SNTP_TIME_SYNCED);

    // Print now accurate current time
    const time_t now = time(NULL);
    struct tm timeInfo;
    char tmBuf[64];
    localtime_r(&now, &timeInfo);
    strftime(tmBuf, sizeof tmBuf, "%Y-%m-%dT%H:%M:%S", &timeInfo);
    ESP_LOGI(TAG, "Time was set. Current datetime is: %s", tmBuf);
}


void start_obtaining_time(void) {
    esp_log_level_set(TAG, ESP_LOG_INFO);
    ESP_LOGI(TAG, "Initializing and starting SNTP");
    sntp_event_group = xEventGroupCreate();
    if (sntp_event_group == NULL) {
        ESP_LOGE(TAG, "Error creating event group! Exiting!");
        esp_restart();
    }

    // Init SNTP with the server given in config
    esp_sntp_config_t sntp_config = ESP_NETIF_SNTP_DEFAULT_CONFIG(config.sntp_time_server);
    sntp_config.sync_cb = time_sync_notification_cb;
    esp_netif_sntp_init(&sntp_config);
}


void wait_for_time_sync(void) {
    ESP_LOGI(TAG, "Checking if time was synced. If not waiting for %d seconds.", SECONDS_TO_WAIT_FOR_TIME);
    const EventBits_t uxBits= xEventGroupWaitBits(sntp_event_group, SNTP_TIME_SYNCED, pdFALSE, pdTRUE, pdMS_TO_TICKS(SECONDS_TO_WAIT_FOR_TIME*1000));
    if((uxBits & SNTP_TIME_SYNCED) == 0 ) {
        ESP_LOGE(TAG, "Time was not synced within %d seconds", SECONDS_TO_WAIT_FOR_TIME);
    }

    // De-Init sntp as it will not be necessary any more
    esp_netif_sntp_deinit();
}
