#include "nvs_helper.h"

#include "configurations.h"

#include "nvs_flash.h"
#include "esp_log.h"
#include <stdio.h>
#include "nvs.h"

static const char *TAG = "[NVS]";

nvs_handle persistent_storage_handle;
const char *namespace_name = "storage";

bool init_nvs(void) {
    esp_log_level_set(TAG, ESP_LOG_INFO);

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) initializing NVS!", esp_err_to_name(err));
        return false;
    }
    ESP_LOGI(TAG, "NVS Flash initialized successfully.");

    err = nvs_open(namespace_name, NVS_READWRITE, &persistent_storage_handle);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
        return false;
    }
    ESP_LOGI(TAG, "NVS handle opened successfully.");

    return true;
}

bool is_key_present(const char *key) {
    const esp_err_t err = nvs_find_key(persistent_storage_handle, key, NULL);
    switch (err) {
        case ESP_OK:
            return true;
        case ESP_ERR_NVS_NOT_FOUND:
            return false;
        default:
            ESP_LOGE(TAG, "Error (%s) finding key '%s'!", esp_err_to_name(err), key);
            return false;
    }
}

void read_value(const char *key, uint32_t *value) {
    const esp_err_t err = nvs_get_u32(persistent_storage_handle, key, value);
    switch (err) {
        case ESP_OK:
            break;
        case ESP_ERR_NVS_NOT_FOUND:
            *value = 0;
            ESP_LOGI(TAG, "The value is not initialized yet! Returning 0 as default");
            break;
        default:
            *value = 0;
            ESP_LOGE(TAG, "Error (%s) reading!", esp_err_to_name(err));
    }
}

bool write_value(const char *key, const uint32_t value) {
    esp_err_t err = nvs_set_u32(persistent_storage_handle, key, value);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) writing!", esp_err_to_name(err));
        return false;
    }

    err = nvs_commit(persistent_storage_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) committing!", esp_err_to_name(err));
        return false;
    }

    return true;
}

void read_string(const char *key, char *value) {
    size_t length = STRINGS_MAX_LENGTH;

    const esp_err_t err = nvs_get_str(persistent_storage_handle, key, value, &length);
    switch (err) {
        case ESP_OK:
            break;
        case ESP_ERR_NVS_NOT_FOUND:
            strncpy(value, "", STRINGS_MAX_LENGTH);
            ESP_LOGI(TAG, "No string found for key '%s'", key);
            break;
        default:
            strncpy(value, "", STRINGS_MAX_LENGTH);
            ESP_LOGE(TAG, "Error (%s) while reading the string for key '%s'!", esp_err_to_name(err), key);
    }
}

bool write_string(const char *key, const char *value) {
    esp_err_t err = nvs_set_str(persistent_storage_handle, key, value);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) while writing the string with key '%s'!", esp_err_to_name(err), key);
        return false;
    }

    err = nvs_commit(persistent_storage_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) while commiting to NVS!", esp_err_to_name(err));
        return false;
    }

    return true;
}


void close_nvs(void) {
    nvs_close(persistent_storage_handle);
}
