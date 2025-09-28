#include "sd_card.h"

#include "configurations.h"

#include "driver/sdmmc_host.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include "cJSON.h"

#define MOUNT_POINT "/sdcard"

const char *configPath = MOUNT_POINT"/config.json";
static const char *TAG = "[SD-Card]";
sdmmc_card_t *card;

/**
 *
 * @return true if the filesystem has been mounted successfully, else false
 */
bool mount_sd_card() {
    esp_log_level_set(TAG, ESP_LOG_INFO);
    ESP_LOGI(TAG, "Initializing SD card");

    const sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.width = 1;
    const esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .allocation_unit_size = 16 * 1024,
        .format_if_mount_failed = false,
        .max_files = 5
    };

    const esp_err_t ret = esp_vfs_fat_sdmmc_mount(MOUNT_POINT, &host, &slot_config, &mount_config, &card);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem.");
        } else {
            ESP_LOGE(TAG, "Failed to initialize the card (%s).", esp_err_to_name(ret));
        }
        return false;
    }

    ESP_LOGI(TAG, "Filesystem successfully mounted");
    return true;
}

/**
 *
 * @param out_size Size of the returned buffer
 * @return Pointer to buffer with file content, NULL if not successful
 */
static char * read_config_file_into_buffer(size_t *out_size) {
    ESP_LOGI(TAG, "Reading file %s", configPath);
    FILE *config_file = fopen(configPath, "rb");
    if (!config_file) {
        ESP_LOGE(TAG, "Fehler beim Öffnen der Datei %s. errno: %d (%s)", configPath, errno, strerror(errno));
        return NULL;
    }

    // Dateigröße ermitteln
    fseek(config_file, 0, SEEK_END);
    const long len = ftell(config_file);
    fseek(config_file, 0, SEEK_SET);
    if (len <= 0) {
        ESP_LOGE(TAG, "Datei %s ist leer oder Fehler bei ftell", configPath);
        fclose(config_file);
        return NULL;
    }
    ESP_LOGD(TAG, "Datei beinhaltet %ld Bytes", len);

    char *buf = malloc(len + 1);
    if (!buf) {
        ESP_LOGE(TAG, "Kein Speicher für Buffer");
        fclose(config_file);
        return NULL;
    }

    // Lesen und terminieren
    const size_t read = fread(buf, 1, len, config_file);
    buf[read] = '\0';
    fclose(config_file);

    *out_size = read;
    return buf;
}

/**
 * Function overrides settings in config if JSON contains the keys
 * @param json JSON-Object containing possible settings to be set
 */
static void extractAndSetInConfig(const cJSON *json) {
    const cJSON *item = NULL;

    item = cJSON_GetObjectItemCaseSensitive(json, "meter_monitor_name");
    if (cJSON_IsString(item) && item->valuestring != NULL) {
        config_set_meter_monitor_name(item->valuestring);
        ESP_LOGD(TAG, "Set meter_monitor_name to: %s", config.meter_monitor_name);
    }

    item = cJSON_GetObjectItemCaseSensitive(json, "sleep_time_min");
    if (cJSON_IsNumber(item)) {
        config_set_sleep_time_min(item->valueint);
        ESP_LOGD(TAG, "Set sleep_time_min to: %d", config.sleep_time_min);
    }

    item = cJSON_GetObjectItemCaseSensitive(json, "wifi_ssid");
    if (cJSON_IsString(item) && item->valuestring != NULL) {
        config_set_wifi_ssid(item->valuestring);
        ESP_LOGD(TAG, "Set wifi_ssid to: %s", config.wifi_ssid);
    }

    item = cJSON_GetObjectItemCaseSensitive(json, "wifi_password");
    if (cJSON_IsString(item) && (item->valuestring != NULL)) {
        config_set_wifi_password(item->valuestring);
        ESP_LOGD(TAG, "Set wifi_password to: %s", config.wifi_password);
    }

    item = cJSON_GetObjectItemCaseSensitive(json, "wifi_maximum_retry");
    if (cJSON_IsNumber(item)) {
        config_set_wifi_maximum_retry(item->valueint);
        ESP_LOGD(TAG, "Set wifi_maximum_retry to: %d", config.wifi_maximum_retry);
    }

    item = cJSON_GetObjectItemCaseSensitive(json, "sntp_time_server");
    if (cJSON_IsString(item) && item->valuestring != NULL) {
        config_set_sntp_time_server(item->valuestring);
        ESP_LOGD(TAG, "Set sntp_time_server to: %s", config.sntp_time_server);
    }

    item = cJSON_GetObjectItemCaseSensitive(json, "sntp_time_sync_always");
    if (cJSON_IsBool(item)) {
        config_set_sntp_time_sync_always(cJSON_IsTrue(item));
        ESP_LOGD(TAG, "Set sntp_time_sync_always to: %s", config.sntp_time_sync_always ? "true" : "false");
    }

    item = cJSON_GetObjectItemCaseSensitive(json, "mqtt_uri");
    if (cJSON_IsString(item) && item->valuestring != NULL) {
        config_set_mqtt_uri(item->valuestring);
        ESP_LOGD(TAG, "Set mqtt_uri to: %s", config.mqtt_uri);
    }

    item = cJSON_GetObjectItemCaseSensitive(json, "mqtt_port");
    if (cJSON_IsNumber(item)) {
        config_set_mqtt_port(item->valueint);
        ESP_LOGD(TAG, "Set mqtt_port to: %d", config.mqtt_port);
    }

    item = cJSON_GetObjectItemCaseSensitive(json, "mqtt_topic_base");
    if (cJSON_IsString(item) && item->valuestring != NULL) {
        config_set_mqtt_topic_base(item->valuestring);
        ESP_LOGD(TAG, "Set mqtt_topic_base to: %s", config.mqtt_topic_base);
    }

    item = cJSON_GetObjectItemCaseSensitive(json, "mqtt_username_defined");
    if (cJSON_IsBool(item)) {
        config_set_mqtt_username_defined(cJSON_IsTrue(item));
        ESP_LOGD(TAG, "Set mqtt_username_defined to: %s", config.mqtt_username_defined ? "true" : "false");
    }

    item = cJSON_GetObjectItemCaseSensitive(json, "mqtt_username");
    if (cJSON_IsString(item) && item->valuestring != NULL) {
        config_set_mqtt_username(item->valuestring);
        ESP_LOGD(TAG, "Set mqtt_username to: %s", config.mqtt_username);
    }

    item = cJSON_GetObjectItemCaseSensitive(json, "mqtt_password_defined");
    if (cJSON_IsBool(item)) {
        config_set_mqtt_password_defined(cJSON_IsTrue(item));
        ESP_LOGD(TAG, "Set mqtt_password_defined to: %s", config.mqtt_password_defined ? "true" : "false");
    }

    item = cJSON_GetObjectItemCaseSensitive(json, "mqtt_password");
    if (cJSON_IsString(item) && item->valuestring != NULL) {
        config_set_mqtt_password(item->valuestring);
        ESP_LOGD(TAG, "Set mqtt_password to: %s", config.mqtt_password);
    }

    item = cJSON_GetObjectItemCaseSensitive(json, "flash_light");
    if (cJSON_IsBool(item)) {
        config_set_flash_light(cJSON_IsTrue(item));
        ESP_LOGD(TAG, "Set flash_light to: %s", config.flash_light ? "true" : "false");
    }

    item = cJSON_GetObjectItemCaseSensitive(json, "flash_gpio");
    if (cJSON_IsNumber(item)) {
        config_set_flash_gpio(item->valueint);
        ESP_LOGD(TAG, "Set flash_gpio to: %d", config.flash_gpio);
    }

    item = cJSON_GetObjectItemCaseSensitive(json, "led_strip");
    if (cJSON_IsBool(item)) {
        config_set_led_strip(cJSON_IsTrue(item));
        ESP_LOGD(TAG, "Set led_strip to: %s", config.led_strip ? "true" : "false");
    }

    item = cJSON_GetObjectItemCaseSensitive(json, "led_strip_gpio");
    if (cJSON_IsNumber(item)) {
        config_set_led_strip_gpio(item->valueint);
        ESP_LOGD(TAG, "Set led_strip_gpio to: %d", config.led_strip_gpio);
    }

    item = cJSON_GetObjectItemCaseSensitive(json, "led_strip_count");
    if (cJSON_IsNumber(item)) {
        config_set_led_strip_count(item->valueint);
        ESP_LOGD(TAG, "Set led_strip_count to: %d", config.led_strip_count);
    }

    item = cJSON_GetObjectItemCaseSensitive(json, "led_r");
    if (cJSON_IsNumber(item)) {
        const int led_r = item->valueint;
        item = cJSON_GetObjectItemCaseSensitive(json, "led_g");
        if (cJSON_IsNumber(item)) {
            const int led_g = item->valueint;
            item = cJSON_GetObjectItemCaseSensitive(json, "led_b");
            if (cJSON_IsNumber(item)) {
                const int led_b = item->valueint;
                config_set_led_color(led_r, led_g, led_b);
                ESP_LOGD(TAG, "Set led_r:%d, led_g:%d, led_b:%d", config.led_r, config.led_g, config.led_b);
            }
        }
    }

    item = cJSON_GetObjectItemCaseSensitive(json, "camera_frame_size");
    if (cJSON_IsString(item) && item->valuestring != NULL) {
        if (strcmp(item->valuestring, "QQVGA") == 0) {
            config_set_camera_frame_size(1);
            ESP_LOGD(TAG, "Set camera_frame_size to: FRAMESIZE_QQVGA");
        } else if (strcmp(item->valuestring, "QVGA") == 0) {
            config_set_camera_frame_size(6);
            ESP_LOGD(TAG, "Set camera_frame_size to: FRAMESIZE_QVGA");
        } else if (strcmp(item->valuestring, "VGA") == 0) {
            config_set_camera_frame_size(10);
            ESP_LOGD(TAG, "Set camera_frame_size to: FRAMESIZE_VGA");
        } else if (strcmp(item->valuestring, "SVGA") == 0) {
            config_set_camera_frame_size(11);
            ESP_LOGD(TAG, "Set camera_frame_size to: FRAMESIZE_SVGA");
        } else if (strcmp(item->valuestring, "XGA") == 0) {
            config_set_camera_frame_size(12);
            ESP_LOGD(TAG, "Set camera_frame_size to: FRAMESIZE_XGA");
        } else if (strcmp(item->valuestring, "SXGA") == 0) {
            config_set_camera_frame_size(14);
            ESP_LOGD(TAG, "Set camera_frame_size to: FRAMESIZE_SXGA");
        } else if (strcmp(item->valuestring, "UXGA") == 0) {
            config_set_camera_frame_size(15);
            ESP_LOGD(TAG, "Set camera_frame_size to: FRAMESIZE_UXGA");
        } else {
            ESP_LOGW(TAG, "Unknown Frame-size: %s", item->valuestring);
        }
    }

    item = cJSON_GetObjectItemCaseSensitive(json, "send_done");
    if (cJSON_IsBool(item)) {
        config_set_send_done(cJSON_IsTrue(item));
        ESP_LOGD(TAG, "Set send_done to: %s", config.send_done ? "true" : "false");
    }

    item = cJSON_GetObjectItemCaseSensitive(json, "done_gpio");
    if (cJSON_IsNumber(item)) {
        config_set_done_gpio(item->valueint);
        ESP_LOGD(TAG, "Set done_gpio to: %d", config.done_gpio);
    }

    ESP_LOGI(TAG, "All settings were successfully extracted from file");
}

/**
 *
 * @return true, if successfully opened the config-file and parsed JSON
 */
bool import_settings_from_file() {
    size_t buf_size;
    char *json_buffer = read_config_file_into_buffer(&buf_size);
    if (!json_buffer) {
        ESP_LOGE(TAG, "Konnte JSON nicht lesen");
        return false;
    }

    cJSON *json = cJSON_Parse(json_buffer);
    free(json_buffer);
    if (!json) {
        ESP_LOGE(TAG, "JSON Parse-Fehler: %s", cJSON_GetErrorPtr());
        return false;
    }

    extractAndSetInConfig(json);

    cJSON_Delete(json);
    return true;
}

/**
 *
 * @return true if the filesystem has been unmounted successfully, else false
 */
bool unmount_sd_card() {
    esp_err_t err = sdmmc_host_deinit();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to deinit SPI host: %s", esp_err_to_name(err));
        return false;
    }
    ESP_LOGI(TAG, "SPI host deinitialized");

    err = esp_vfs_fat_sdcard_unmount(MOUNT_POINT, card);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to unmount filesystem: %s", esp_err_to_name(err));
        return false;
    }
    ESP_LOGI(TAG, "Filesystem unmounted");

    return true;
}
