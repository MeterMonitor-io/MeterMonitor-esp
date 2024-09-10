#ifndef MQTT_H
#define MQTT_H

void start_mqtt(void);
void publish_message(const char *msg);
void stop_mqtt(void);

#endif // MQTT_H