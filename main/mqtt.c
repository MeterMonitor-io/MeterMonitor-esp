#include "mqtt.h"

#include "configurations.h"

#include "lwip/sockets.h"
#include "mqtt_client.h"
#include "esp_netif.h"
#include "esp_log.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>

static const char *TAG = "[MQTT]";
esp_mqtt_client_handle_t client;
const int secondsToTry = 25;
bool brokerConnected = false;
bool messageSent = false;


static void log_error_if_nonzero(const char *message, int error_code) {
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}


static void mqtt_event_handler(__attribute__((unused)) void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_BEFORE_CONNECT:
            ESP_LOGI(TAG, "MQTT_EVENT_BEFORE_CONNECT");
            break;
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            brokerConnected = true;
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            messageSent = true;
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
    esp_mqtt_client_config_t mqtt_cfg = {
            .broker.address.uri = config.mqtt_uri,
            .broker.address.port = config.mqtt_port,
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
    int retry = 0;
    while (brokerConnected != true && ++retry < secondsToTry) {
        ESP_LOGI(TAG, "Waiting for broker connection... (%d/%d)", retry, secondsToTry);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    // Construct Topic-String
    char topic[STRINGS_MAX_LENGTH * 2 + 2];
    topic[0] = '\0';
    strncat(topic, config.mqtt_topic_base, sizeof(topic)-strlen(topic)-1);
    strncat(topic, config.meter_monitor_name, sizeof(topic)-strlen(topic)-1);
    strncat(topic, "/", sizeof(topic)-strlen(topic)-1);

    // Publish MQTT message to the right topic (with QOS 2)
    ESP_LOGI(TAG, "Sending message to %s", topic);
    esp_mqtt_client_publish(client, topic, msg, 0, 2, 0);

    // Make sure the Message was sent before continuing
    retry = 0;
    while (messageSent != true && ++retry < secondsToTry) {
        ESP_LOGI(TAG, "Waiting for message to be sent... (%d/%d)", retry, secondsToTry);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    // Reset global variable for the next iteration (Probably not needed due to sleep, but better to be sure)
    messageSent = false;
}


void stop_mqtt(void) {
    ESP_ERROR_CHECK(esp_mqtt_client_disconnect(client));

    // Reset global variable for the next iteration (Probably not needed due to sleep, but better to be sure)
    brokerConnected = false;
}
