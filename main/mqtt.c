#include "mqtt.h"

#include "configurations.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "mqtt_client.h"
#include "esp_netif.h"
#include "esp_log.h"
#include <string.h>

#define MQTT_CONNECTED_BIT      BIT0
#define MQTT_PUBLISHED_BIT      BIT1
static EventGroupHandle_t mqtt_event_group;
static const char *TAG = "[MQTT]";
esp_mqtt_client_handle_t client;
const int secondsToTry = 30;


static void log_error_if_nonzero(const char *message, const int error_code) {
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}


static void mqtt_event_handler(__attribute__((unused)) void *handler_args, esp_event_base_t base, const int32_t event_id, void *event_data) {
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%ld", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_BEFORE_CONNECT:
            ESP_LOGI(TAG, "MQTT_EVENT_BEFORE_CONNECT");
            break;
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            xEventGroupSetBits(mqtt_event_group, MQTT_CONNECTED_BIT);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            xEventGroupClearBits(mqtt_event_group, MQTT_CONNECTED_BIT);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            xEventGroupSetBits(mqtt_event_group, MQTT_PUBLISHED_BIT);
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
    mqtt_event_group = xEventGroupCreate();
    if (mqtt_event_group == NULL) {
        ESP_LOGE(TAG, "Error creating event group! Exiting!");
        esp_restart();
    }

    ESP_LOGI(TAG, "Starting MQTT-Connection");
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = config.mqtt_uri,
        .broker.address.port = config.mqtt_port,
        .buffer.out_size = 1024 * 64
    };
    if (config.mqtt_username_defined) {
        mqtt_cfg.credentials.username = config.mqtt_username;
    }
    if (config.mqtt_password_defined) {
        mqtt_cfg.credentials.authentication.password = config.mqtt_password;
    }

    client = esp_mqtt_client_init(&mqtt_cfg);
    if (client == NULL) {
        ESP_LOGE(TAG, "MQTT could not be initialised!");
    }
    ESP_ERROR_CHECK(esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL));
    ESP_ERROR_CHECK(esp_mqtt_client_start(client));
}


void publish_message(const char *msg) {
    // Make sure MQTT-Broker is connected
    EventBits_t uxBits= xEventGroupWaitBits(mqtt_event_group, MQTT_CONNECTED_BIT, pdFALSE, pdTRUE, pdMS_TO_TICKS(secondsToTry*1000));
    if((uxBits & MQTT_CONNECTED_BIT) == 0 ) {
        ESP_LOGE(TAG, "MQTT-connection could not be established within %d seconds", secondsToTry);
    }

    // Construct Topic-String
    char topic[STRINGS_MAX_LENGTH * 2 + 2];
    topic[0] = '\0';
    strncat(topic, config.mqtt_topic_base, sizeof(topic)-strlen(topic)-1);
    strncat(topic, config.meter_monitor_name, sizeof(topic)-strlen(topic)-1);
    strncat(topic, "/", sizeof(topic)-strlen(topic)-1);

    const uint32_t msg_len = strlen(msg);
    // Publish MQTT message to the right topic (with QOS 1 to save on time)
    ESP_LOGI(TAG, "Sending message to %s with len=%lu", topic, msg_len);
    esp_mqtt_client_publish(client, topic, msg, (int) msg_len, 1, 0);

    // Make sure the Message was sent before continuing
    uxBits = xEventGroupWaitBits(mqtt_event_group, MQTT_PUBLISHED_BIT, pdTRUE, pdFALSE, pdMS_TO_TICKS(secondsToTry*1000));
    if((uxBits & MQTT_PUBLISHED_BIT) == 0 ) {
        ESP_LOGE(TAG, "Message was not published within %d seconds", secondsToTry);
    }
}


void stop_mqtt(void) {
    ESP_LOGI(TAG, "Disconnecting from MQTT-Broker");
    ESP_ERROR_CHECK(esp_mqtt_client_disconnect(client));
}
