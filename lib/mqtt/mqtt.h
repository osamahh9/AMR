#ifndef MQTT_H
#define MQTT_H

#include "esp_err.h"

/**
 * @brief Initialize and start the MQTT client
 */
void mqtt_app_start(void);

/**
 * @brief Publish an event to cell/amr/event
 * 
 * @param event The event name (e.g. "AMR_PICK_COMMAND_ACCEPTED")
 * @param jobId The associated jobId
 */
void mqtt_publish_event(const char *event, const char *jobId);

/**
 * @brief Publish a status update to cell/amr/status
 */
void mqtt_publish_status(void);

/**
 * @brief Called by control task when a navigation mission is completed
 */
void mqtt_report_completion(void);

/**
 * @brief Check if the MQTT client is connected
 * 
 * @return true If connected
 * @return false If disconnected
 */
bool mqtt_is_connected(void);

extern char last_mqtt_cmd[128];

#endif // MQTT_H
