#include "mqtt.h"

#include "lwip/sockets.h"
#include "mqtt_client.h"
#include "esp_netif.h"
#include "esp_log.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#define MQTT_URI            CONFIG_WATER_METER_MQTT_URI
#define MQTT_PORT           CONFIG_WATER_METER_MQTT_PORT
#define MQTT_TOPIC          CONFIG_WATER_METER_MQTT_TOPIC
#define MQTT_USERNAME       CONFIG_WATER_METER_MQTT_USERNAME
#define MQTT_PASSWORD       CONFIG_WATER_METER_MQTT_PASSWORD

static const char *TAG = "MQTT-Controller";
esp_mqtt_client_handle_t client;

static void log_error_if_nonzero(const char *message, int error_code) {
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

static void mqtt_event_handler(__attribute__((unused)) void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
                log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
                log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
                log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
                ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
            }
            break;
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
}

void start_mqtt(void) {
    esp_log_level_set(TAG, ESP_LOG_INFO);

    ESP_LOGI(TAG, "Starting MQTT-Connection");
    const esp_mqtt_client_config_t mqtt_cfg = {
            .broker.address.uri = MQTT_URI,
            .broker.address.port = MQTT_PORT,
#ifdef CONFIG_WATER_METER_MQTT_USERNAME_DEFINED
            .credentials.username = MQTT_USERNAME,
#endif
#ifdef CONFIG_WATER_METER_MQTT_PASSWORD_DEFINED
            .credentials.authentication.password = MQTT_PASSWORD,
#endif
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    if (client == NULL) {
        ESP_LOGE(TAG, "MQTT could not be initialised!");
    }
    ESP_ERROR_CHECK(esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL));
    ESP_ERROR_CHECK(esp_mqtt_client_start(client));
}

void publish_message(const char *msg) {
    vTaskDelay(pdMS_TO_TICKS(100));
    esp_mqtt_client_publish(client, MQTT_TOPIC, msg, 0, 2, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
}

void stop_mqtt(void) {
    vTaskDelay(pdMS_TO_TICKS(100));
    ESP_ERROR_CHECK(esp_mqtt_client_disconnect(client));
}
