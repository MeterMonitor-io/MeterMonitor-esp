#include "sntp.h"

#include "esp_netif_sntp.h"
#include "nvs_flash.h"
#include "esp_attr.h"
#include "esp_log.h"
#include <time.h>

#define SNTP_SERVER         CONFIG_METER_MONITOR_SNTP_TIME_SERVER

static const char *TAG = "[SNTP]";
const int maxRetries = 20;


void time_sync_notification_cb(__attribute__((unused)) struct timeval *tv) {
    ESP_LOGI(TAG, "Time has been synced successfully via the SNTP-Server.");
}


void obtain_time(void) {
    ESP_LOGI(TAG, "Initializing and starting SNTP");

    // Init SNTP with server given in config
    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG(SNTP_SERVER);
    config.sync_cb = time_sync_notification_cb;
    esp_netif_sntp_init(&config);

    // wait for time to be set
    time_t now = 0;
    struct tm timeInfo = { 0 };
    int retry = 0;
    while (esp_netif_sntp_sync_wait(2000 / portTICK_PERIOD_MS) == ESP_ERR_TIMEOUT && ++retry < maxRetries) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, maxRetries);
    }

    // Print now accurate current time
    char tmBuf[64];
    time(&now);
    localtime_r(&now, &timeInfo);
    strftime(tmBuf, sizeof tmBuf, "%Y-%m-%dT%H:%M:%S", &timeInfo);
    ESP_LOGI(TAG, "Time was set. Current date and time is: %s", tmBuf);

    // De-Init sntp as it will not be necessary any more
    esp_netif_sntp_deinit();
}
