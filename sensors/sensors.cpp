// sensors/sensors.cpp
#include "sensors.h"
#include <Wire.h>
#include <Adafruit_AHTX0.h>

Adafruit_AHTX0 aht;

volatile float currentTemperature = 0.0;
volatile float currentHumidity = 0.0;

void initSensors() {
  Wire.begin(21, 22);
  if (!aht.begin()) {
    Serial.println("AHT10 not found");
    while (1) delay(10);
  }
  Serial.println("AHT10 initialized");
}

void sensorTask(void *pvParameters) {
  sensors_event_t humidity, temp;
  for (;;) {
    aht.getEvent(&humidity, &temp);
    currentTemperature = temp.temperature;
    currentHumidity = humidity.relative_humidity;
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void initSensorTask() {
  xTaskCreate(sensorTask, "SensorTask", 4096, NULL, 1, NULL);
}