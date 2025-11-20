#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "sdkconfig.h"

#define STRINGS_MAX_LENGTH  64
#define GPIO_MAX            39


/**
 *  Values that need to be defined for program logic.
 */
//----------------------------------------------------- General defines --------
#ifndef CONFIG_METER_MONITOR_NAME
    #define CONFIG_METER_MONITOR_NAME                   "UndefinedMeter"
#endif
#ifndef CONFIG_METER_MONITOR_SLEEP_TIME
    #define CONFIG_METER_MONITOR_SLEEP_TIME             5
#endif
//----------------------------------------------------- WiFi defines -----------
#ifndef CONFIG_METER_MONITOR_WIFI_SSID
    #define CONFIG_METER_MONITOR_WIFI_SSID              "myssid"
#endif
#ifndef CONFIG_METER_MONITOR_WIFI_PASSWORD
    #define CONFIG_METER_MONITOR_WIFI_PASSWORD          "mypassword"
#endif
#ifndef CONFIG_METER_MONITOR_WIFI_MAXIMUM_RETRY
    #define CONFIG_METER_MONITOR_WIFI_MAXIMUM_RETRY     5
#endif
//----------------------------------------------------- SNTP defines -----------
#ifndef CONFIG_METER_MONITOR_SNTP_TIME_SERVER
    #define CONFIG_METER_MONITOR_SNTP_TIME_SERVER       "pool.ntp.org"
#endif
#ifndef CONFIG_METER_MONITOR_SNTP_TIME_SYNC_ALWAYS
    #define CONFIG_METER_MONITOR_SNTP_TIME_SYNC_ALWAYS  false
#endif
//----------------------------------------------------- MQTT defines ----------
#ifndef CONFIG_METER_MONITOR_MQTT_URI
    #define CONFIG_METER_MONITOR_MQTT_URI               ""
#endif
#ifndef CONFIG_METER_MONITOR_MQTT_PORT
    #define CONFIG_METER_MONITOR_MQTT_PORT              1883
#endif
#ifndef CONFIG_METER_MONITOR_MQTT_TOPIC_BASE
    #define CONFIG_METER_MONITOR_MQTT_TOPIC_BASE        "/"
#endif
#ifndef CONFIG_METER_MONITOR_MQTT_USERNAME_DEFINED
    #define CONFIG_METER_MONITOR_MQTT_USERNAME_DEFINED  false
#endif
#ifndef CONFIG_METER_MONITOR_MQTT_PASSWORD_DEFINED
    #define CONFIG_METER_MONITOR_MQTT_PASSWORD_DEFINED  false
#endif
//----------------------------------------------------- Flash defines ----------
#ifndef CONFIG_METER_MONITOR_FLASH
    #define CONFIG_METER_MONITOR_FLASH                  false
#endif
//----------------------------------------------------- LED-Strip defines ------
#ifndef CONFIG_METER_MONITOR_LED_STRIP
    #define CONFIG_METER_MONITOR_LED_STRIP              false
#endif
//----------------------------------------------------- camera setting defines -
#define CAMERA_BACKUP_FRAME_SIZE                        10
//----------------------------------------------------- Done defines -----------
#ifndef CONFIG_METER_MONITOR_DONE
    #define CONFIG_METER_MONITOR_DONE                   false
#endif


typedef struct {
    //------------------------------------------------- General settings
    char meter_monitor_name[STRINGS_MAX_LENGTH];
    int sleep_time_min;
    //------------------------------------------------- WiFi settings
    char wifi_ssid[STRINGS_MAX_LENGTH];
    char wifi_password[STRINGS_MAX_LENGTH];
    int wifi_maximum_retry;
    //------------------------------------------------- SNTP settings
    char sntp_time_server[STRINGS_MAX_LENGTH];
    bool sntp_time_sync_always;
    //------------------------------------------------- MQTT settings
    char mqtt_uri[STRINGS_MAX_LENGTH];
    int  mqtt_port;
    char mqtt_topic_base[STRINGS_MAX_LENGTH];
    bool mqtt_username_defined;
    char mqtt_username[STRINGS_MAX_LENGTH];
    bool mqtt_password_defined;
    char mqtt_password[STRINGS_MAX_LENGTH];
    //------------------------------------------------- Flash settings
    bool flash_light;
    int flash_gpio;
    //------------------------------------------------- LED-Strip settings
    bool led_strip;
    int led_strip_gpio;
    int led_strip_count;
    int led_r, led_g, led_b;
    //------------------------------------------------- Camera settings
    int camera_frame_size;
    //------------------------------------------------- Done-signal settings
    bool send_done;
    int done_gpio;
} configurations_t;


extern configurations_t config;


void init_config(void);

bool config_set_meter_monitor_name(const char *name);
bool config_set_sleep_time_min(int minutes);

bool config_set_wifi_ssid(const char *name);
bool config_set_wifi_password(const char *password);
bool config_set_wifi_maximum_retry(int retries);

bool config_set_sntp_time_server(const char *server);
bool config_set_sntp_time_sync_always(bool sync_always);

bool config_set_mqtt_uri(const char *uri);
bool config_set_mqtt_port(int port);
bool config_set_mqtt_topic_base(const char *topic);
bool config_set_mqtt_username(const char *username);
bool config_set_mqtt_password(const char *password);
bool config_set_mqtt_username_defined(bool defined);
bool config_set_mqtt_password_defined(bool defined);

bool config_set_flash_light(bool enabled);
bool config_set_flash_gpio(int gpio);

bool config_set_led_strip(bool enabled);
bool config_set_led_strip_gpio(int gpio);
bool config_set_led_strip_count(int count);
bool config_set_led_color(int r, int g, int b);

bool config_set_camera_frame_size(int frameSize);

bool config_set_send_done(bool enabled);
bool config_set_done_gpio(int gpio);

// NVS store-and-retrieve-functions
bool check_if_config_in_nvs();
bool read_config_from_nvs();
bool write_config_to_nvs();
