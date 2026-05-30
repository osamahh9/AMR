#ifndef OBJECT_H
#define OBJECT_H

#include "ultrasonic.h"
#include "iot_servo.h"

extern volatile uint32_t measured_distance;
extern volatile bool obstacle_detected;

/**
 * @brief Initialize the object detection system (ultrasonic and servos)
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t object_detection_init(void);

/**
 * @brief Task to monitor distance and perform dummy servo movements
 * 
 * @param pvParameters 
 */
void object_detection_task(void *pvParameters);

/**
 * @brief Set servo angle externally
 * 
 * @param channel 0 or 1
 * @param angle 0 to 180
 */
void object_set_servo(uint8_t channel, float angle);

#endif // OBJECT_H
