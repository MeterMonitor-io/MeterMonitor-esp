#include "nvs_helper.h"

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
    } else {
        ESP_LOGI(TAG, "NVS Flash initialized successfully.");
    }

    err = nvs_open(namespace_name, NVS_READWRITE, &persistent_storage_handle);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
        return false;
    } else {
        ESP_LOGI(TAG, "NVS handle opened successfully.");
        return true;
    }
}

void read_value(const char *key, uint32_t *value) {
    esp_err_t err = nvs_get_u32(persistent_storage_handle, key, value);
    switch (err) {
        case ESP_OK:
            ESP_LOGI(TAG, "Got value = %"PRIu32" with key %s", *value, key);
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

bool write_value(const char *key, uint32_t value) {
    esp_err_t err = nvs_set_u32(persistent_storage_handle, key, value);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) writing!", esp_err_to_name(err));
        return false;
    } else {
        ESP_LOGI(TAG, "Successfully updated value (%"PRIu32") for key '%s' in NVS", value, key);
    }

    err = nvs_commit(persistent_storage_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) committing!", esp_err_to_name(err));
        return false;
    } else {
        ESP_LOGI(TAG, "Successfully committed updates in NVS");
        return true;
    }
}

void close_nvs(void) {
    nvs_close(persistent_storage_handle);
}
