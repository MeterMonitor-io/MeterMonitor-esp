#include "configurations.h"

#include "nvs_helper.h"
#include <string.h>

configurations_t config;

void init_config(void) {
    // General default config
    config_set_meter_monitor_name(CONFIG_METER_MONITOR_NAME);
    config_set_sleep_time_min(CONFIG_METER_MONITOR_SLEEP_TIME);

    // WiFi default config
    config_set_wifi_ssid(CONFIG_METER_MONITOR_WIFI_SSID);
    config_set_wifi_password(CONFIG_METER_MONITOR_WIFI_PASSWORD);
    config_set_wifi_maximum_retry(CONFIG_METER_MONITOR_WIFI_MAXIMUM_RETRY);

    // SNTP default config
    config_set_sntp_time_server(CONFIG_METER_MONITOR_SNTP_TIME_SERVER);
    config_set_sntp_time_sync_always(CONFIG_METER_MONITOR_SNTP_TIME_SYNC_ALWAYS);

    // MQTT default config
    config_set_mqtt_uri(CONFIG_METER_MONITOR_MQTT_URI);
    config_set_mqtt_port(CONFIG_METER_MONITOR_MQTT_PORT);
    config_set_mqtt_topic_base(CONFIG_METER_MONITOR_MQTT_TOPIC_BASE);
    config_set_mqtt_username_defined(CONFIG_METER_MONITOR_MQTT_USERNAME_DEFINED);
#if CONFIG_METER_MONITOR_MQTT_USERNAME_DEFINED
    config_set_mqtt_username(CONFIG_METER_MONITOR_MQTT_USERNAME);
#endif
    config_set_mqtt_password_defined(CONFIG_METER_MONITOR_MQTT_PASSWORD_DEFINED);
#if CONFIG_METER_MONITOR_MQTT_PASSWORD_DEFINED
    config_set_mqtt_password(CONFIG_METER_MONITOR_MQTT_PASSWORD);
#endif

    // Flash default config
    config_set_flash_light(CONFIG_METER_MONITOR_FLASH);
#if CONFIG_METER_MONITOR_FLASH
    config_set_flash_gpio(CONFIG_METER_MONITOR_FLASH_GPIO);
#endif

    // LED-Strip default config
    config_set_led_strip(CONFIG_METER_MONITOR_LED_STRIP);
#if CONFIG_METER_MONITOR_LED_STRIP
    config_set_led_strip_gpio(CONFIG_METER_MONITOR_LED_STRIP_GPIO);
    config_set_led_strip_count(CONFIG_METER_MONITOR_LED_STRIP_LED_COUNT);
    config_set_led_color(CONFIG_METER_MONITOR_LED_STRIP_R,
                         CONFIG_METER_MONITOR_LED_STRIP_G,
                         CONFIG_METER_MONITOR_LED_STRIP_B);
#endif

    // camera-settings default config
#ifdef CONFIG_METER_MONITOR_CAMERA_FRAME_SIZE_QQVGA
    config_set_camera_frame_size(1);
#elifdef CONFIG_METER_MONITOR_CAMERA_FRAME_SIZE_QVGA
    config_set_camera_frame_size(6);
#elifdef CONFIG_METER_MONITOR_CAMERA_FRAME_SIZE_VGA
    config_set_camera_frame_size(10);
#elifdef CONFIG_METER_MONITOR_CAMERA_FRAME_SIZE_SVGA
    config_set_camera_frame_size(11);
#elifdef CONFIG_METER_MONITOR_CAMERA_FRAME_SIZE_XGA
    config_set_camera_frame_size(12);
#elifdef CONFIG_METER_MONITOR_CAMERA_FRAME_SIZE_SXGA
    config_set_camera_frame_size(14);
#elifdef CONFIG_METER_MONITOR_CAMERA_FRAME_SIZE_UXGA
    config_set_camera_frame_size(15);
#else
    config_set_camera_frame_size(CAMERA_BACKUP_FRAME_SIZE); // default camera frame size
#endif

    // Done-signal default config
    config_set_send_done(CONFIG_METER_MONITOR_DONE);
#if CONFIG_METER_MONITOR_DONE
    config_set_done_gpio(CONFIG_METER_MONITOR_DONE_GPIO);
#endif
}

bool config_set_meter_monitor_name(const char *name) {
    if (name == NULL || strlen(name) == 0 || strlen(name) >= STRINGS_MAX_LENGTH) {
        return false;
    }
    strncpy(config.meter_monitor_name, name, STRINGS_MAX_LENGTH - 1);
    config.meter_monitor_name[STRINGS_MAX_LENGTH - 1] = '\0';
    return true;
}

bool config_set_sleep_time_min(const int minutes) {
    if (minutes <= 0) {
        return false;
    }
    config.sleep_time_min = minutes;
    return true;
}

bool config_set_wifi_ssid(const char *name) {
    if (name == NULL || strlen(name) >= STRINGS_MAX_LENGTH) {
        return false;
    }
    strncpy(config.wifi_ssid, name, STRINGS_MAX_LENGTH - 1);
    config.wifi_ssid[STRINGS_MAX_LENGTH - 1] = '\0';
    return true;
}

bool config_set_wifi_password(const char *password) {
    if (password == NULL || strlen(password) >= STRINGS_MAX_LENGTH) {
        return false;
    }
    strncpy(config.wifi_password, password, STRINGS_MAX_LENGTH - 1);
    config.wifi_password[STRINGS_MAX_LENGTH - 1] = '\0';
    return true;
}

bool config_set_wifi_maximum_retry(const int retries) {
    if (retries < 0 || retries > 100) {
        return false;
    }
    config.wifi_maximum_retry = retries;
    return true;
}

bool config_set_sntp_time_server(const char *server) {
    if (server == NULL || strlen(server) >= STRINGS_MAX_LENGTH) {
        return false;
    }
    strncpy(config.sntp_time_server, server, STRINGS_MAX_LENGTH - 1);
    config.sntp_time_server[STRINGS_MAX_LENGTH - 1] = '\0';
    return true;
}

bool config_set_sntp_time_sync_always(const bool sync_always) {
    config.sntp_time_sync_always = sync_always;
    return true;
}

bool config_set_mqtt_uri(const char *uri) {
    if (uri == NULL || strlen(uri) >= STRINGS_MAX_LENGTH) {
        return false;
    }
    strncpy(config.mqtt_uri, uri, STRINGS_MAX_LENGTH - 1);
    config.mqtt_uri[STRINGS_MAX_LENGTH - 1] = '\0';
    return true;
}

bool config_set_mqtt_port(const int port) {
    if (port <= 0 || port > 65535) {
        return false;
    }
    config.mqtt_port = port;
    return true;
}

bool config_set_mqtt_topic_base(const char *topic) {
    if (topic == NULL || strlen(topic) >= STRINGS_MAX_LENGTH) {
        return false;
    }
    strncpy(config.mqtt_topic_base, topic, STRINGS_MAX_LENGTH - 1);
    config.mqtt_topic_base[STRINGS_MAX_LENGTH - 1] = '\0';
    return true;
}

bool config_set_mqtt_username_defined(const bool defined) {
    config.mqtt_username_defined = defined;
    return true;
}

bool config_set_mqtt_username(const char *username) {
    if (username == NULL || strlen(username) >= STRINGS_MAX_LENGTH) {
        return false;
    }
    strncpy(config.mqtt_username, username, STRINGS_MAX_LENGTH - 1);
    config.mqtt_username[STRINGS_MAX_LENGTH - 1] = '\0';
    return true;
}

bool config_set_mqtt_password_defined(const bool defined) {
    config.mqtt_password_defined = defined;
    return true;
}

bool config_set_mqtt_password(const char *password) {
    if (password == NULL || strlen(password) >= STRINGS_MAX_LENGTH) {
        return false;
    }
    strncpy(config.mqtt_password, password, STRINGS_MAX_LENGTH - 1);
    config.mqtt_password[STRINGS_MAX_LENGTH - 1] = '\0';
    return true;
}

bool config_set_flash_light(const bool enabled) {
    config.flash_light = enabled;
    return true;
}

bool config_set_flash_gpio(const int gpio) {
    if (gpio < 0 || gpio > GPIO_MAX) {
        return false;
    }
    config.flash_gpio = gpio;
    return true;
}

bool config_set_led_strip(const bool enabled) {
    config.led_strip = enabled;
    return true;
}

bool config_set_led_strip_gpio(const int gpio) {
    if (gpio < 0 || gpio > GPIO_MAX) {
        return false;
    }
    config.led_strip_gpio = gpio;
    return true;
}

bool config_set_led_strip_count(const int count) {
    if (count < 0 || count > 100) {
        return false;
    }
    config.led_strip_count = count;
    return true;
}

bool config_set_led_color(const int r, const int g, const int b) {
    if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255) {
        return false;
    }
    config.led_r = r;
    config.led_g = g;
    config.led_b = b;
    return true;
}

bool config_set_camera_frame_size(const int frameSize) {
    config.camera_frame_size = frameSize;
    return true;
}

bool config_set_send_done(const bool enabled) {
    config.send_done = enabled;
    return true;
}

bool config_set_done_gpio(const int gpio) {
    if (gpio < 0 || gpio > GPIO_MAX) {
        return false;
    }
    config.done_gpio = gpio;
    return true;
}

bool check_if_config_in_nvs() {
    if (!is_key_present("meter_name")) return false;
    return true;
}

bool read_config_from_nvs() {
    read_string("meter_name", config.meter_monitor_name);
    read_value("sleep_time_min", (uint32_t *) &config.sleep_time_min);

    read_string("wifi_ssid", config.wifi_ssid);
    read_string("wifi_password", config.wifi_password);
    read_value("wifi_max_try", (uint32_t *) &config.wifi_maximum_retry);

    read_string("sntp_time_svr", config.sntp_time_server);
    read_value("sntp_syncAlways", (uint32_t *) &config.sntp_time_sync_always);

    read_string("mqtt_uri", config.mqtt_uri);
    read_value("mqtt_port", (uint32_t *) &config.mqtt_port);
    read_string("mqtt_topic_base", config.mqtt_topic_base);
    read_value("mqtt_usr_def", (uint32_t *) &config.mqtt_username_defined);
    read_string("mqtt_username", config.mqtt_username);
    read_value("mqtt_pwd_def", (uint32_t *) &config.mqtt_password_defined);
    read_string("mqtt_password", config.mqtt_password);

    read_value("flash_light", (uint32_t *) &config.flash_light);
    read_value("flash_gpio", (uint32_t *) &config.flash_gpio);

    read_value("led_strip", (uint32_t *) &config.led_strip);
    read_value("led_strip_gpio", (uint32_t *) &config.led_strip_gpio);
    read_value("led_strip_count", (uint32_t *) &config.led_strip_count);
    read_value("led_r", (uint32_t *) &config.led_r);
    read_value("led_g", (uint32_t *) &config.led_g);
    read_value("led_b", (uint32_t *) &config.led_b);

    read_value("camera_frame", (uint32_t *) &config.camera_frame_size);

    read_value("send_done", (uint32_t *) &config.send_done);
    read_value("done_gpio", (uint32_t *) &config.done_gpio);

    return true;
}

bool write_config_to_nvs() {
    if (!write_string("meter_name", config.meter_monitor_name)) return false;
    if (!write_value("sleep_time_min", config.sleep_time_min)) return false;

    if (!write_string("wifi_ssid", config.wifi_ssid)) return false;
    if (!write_string("wifi_password", config.wifi_password)) return false;
    if (!write_value("wifi_max_try", config.wifi_maximum_retry)) return false;

    if (!write_string("sntp_time_svr", config.sntp_time_server)) return false;
    if (!write_value("sntp_syncAlways", config.sntp_time_sync_always)) return false;

    if (!write_string("mqtt_uri", config.mqtt_uri)) return false;
    if (!write_value("mqtt_port", config.mqtt_port)) return false;
    if (!write_string("mqtt_topic_base", config.mqtt_topic_base)) return false;
    if (!write_value("mqtt_usr_def", config.mqtt_username_defined)) return false;
    if (!write_string("mqtt_username", config.mqtt_username)) return false;
    if (!write_value("mqtt_pwd_def", config.mqtt_password_defined)) return false;
    if (!write_string("mqtt_password", config.mqtt_password)) return false;

    if (!write_value("flash_light", config.flash_light)) return false;
    if (!write_value("flash_gpio", config.flash_gpio)) return false;

    if (!write_value("led_strip", config.led_strip)) return false;
    if (!write_value("led_strip_gpio", config.led_strip_gpio)) return false;
    if (!write_value("led_strip_count", config.led_strip_count)) return false;
    if (!write_value("led_r", config.led_r)) return false;
    if (!write_value("led_g", config.led_g)) return false;
    if (!write_value("led_b", config.led_b)) return false;

    if (!write_value("camera_frame", config.camera_frame_size)) return false;

    if (!write_value("send_done", config.send_done)) return false;
    if (!write_value("done_gpio", config.done_gpio)) return false;

    return true;
}
