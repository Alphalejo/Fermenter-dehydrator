// sensors.h
#pragma once
#include <Arduino.h>

extern volatile float currentTemperature;
extern volatile float currentHumidity;
extern volatile bool sensorMismatchError;

void initSensors();
void initSensorTask();