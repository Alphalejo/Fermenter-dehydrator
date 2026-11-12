// sensors.h
#pragma once
#include <Arduino.h>

extern volatile float currentTemperature;
extern volatile float currentHumidity;
extern volatile bool sensorMismatchError;
extern volatile float sht1_temp;
extern volatile float sht1_hum;
extern volatile float sht2_temp;
extern volatile float sht2_hum;
extern volatile float humidityEntrance;
extern volatile float humidityOutput;
extern volatile float tempEntrance;
extern volatile float tempOutput;

void initSensors();
void initSensorTask();