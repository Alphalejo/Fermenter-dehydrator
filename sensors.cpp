// sensors.cpp
#include "sensors.h"
#include <Wire.h>
#include <Adafruit_SHT31.h>
#include <Adafruit_AHTX0.h> // New library for AHT10

#define SHT31_ADDR_1 0x44
#define SHT31_ADDR_2 0x45

Adafruit_SHT31 sht1 = Adafruit_SHT31();
Adafruit_SHT31 sht2 = Adafruit_SHT31();

// New AHT10 instances
Adafruit_AHTX0 aht_in;
Adafruit_AHTX0 aht_out;

// Shared variables, read by displayTask and actuators
volatile float currentTemperature = 0.0;
volatile float currentHumidity = 0.0;
volatile bool sensorMismatchError = false;

volatile float sht1_temp = 0.0;
volatile float sht1_hum = 0.0;
volatile float sht2_temp = 0.0;
volatile float sht2_hum = 0.0;

// New shared variables for dehydrator airflow (Add these to sensors.h as well using 'extern')
volatile float humidityEntrance = 0.0;
volatile float humidityOutput = 0.0;
volatile float tempEntrance = 0.0;
volatile float tempOutput = 0.0;

void initSensors() {
  // 1. Initialize Main I2C Bus (SDA=21, SCL=22)
  Wire.begin(21, 22); 
  delay(100);         
  
  // 2. Initialize Second I2C Bus (SDA=32, SCL=33) for the second AHT10
  Wire1.begin(32, 33);
  delay(100);

  // Initialize SHT31s (they default to the main 'Wire' bus)
  if (!sht1.begin(SHT31_ADDR_1)) {
    Serial.println("SHT31 #1 (0x44) not found");
  } 
  if (!sht2.begin(SHT31_ADDR_2)) {
    Serial.println("SHT31 #2 (0x45) not found");
  }

  // Initialize AHT10 #1 (Air Entrance) on the main bus
  if (!aht_in.begin(&Wire)) {
    Serial.println("AHT10 IN (0x38) not found on pins 21/22");
  } else {
    Serial.println("AHT10 IN initialized");
  }

  // Initialize AHT10 #2 (Air Output) on the second bus (Wire1)
  if (!aht_out.begin(&Wire1)) {
    Serial.println("AHT10 OUT (0x38) not found on pins 32/33");
  } else {
    Serial.println("AHT10 OUT initialized");
  }
}

void sensorTask(void *pvParameters) {
  for (;;) {
    // --- 1. Read SHT31 Sensors (Redundancy Logic) ---
    float t1 = sht1.readTemperature();
    float h1 = sht1.readHumidity();
    float t2 = sht2.readTemperature();
    float h2 = sht2.readHumidity();

    sht1_temp = t1;
    sht1_hum = h1;
    sht2_temp = t2;
    sht2_hum = h2;

    if (isnan(t1) || isnan(t2) || isnan(h1) || isnan(h2)) {
      sensorMismatchError = true;
      Serial.println("ALERT: SHT31 sensor read failure (NaN).");
    } else {
      bool excessiveDiff = abs(t1 - t2) > 10.0;
      bool extremeTemp = (t1 < 0.0 || t1 > 110.0 || t2 < 0.0 || t2 > 110.0);

      if (excessiveDiff || extremeTemp) {
        sensorMismatchError = true;
      } else {
        sensorMismatchError = false;
        currentTemperature = (t1 + t2) / 2.0;
        currentHumidity = (h1 + h2) / 2.0;
      }
    }

    // --- 2. Read AHT10 Sensors (Dehydrator Airflow) ---
    sensors_event_t humidity_in_event, temp_in_event;
    sensors_event_t humidity_out_event, temp_out_event;
    
    // The AHTX0 library uses a slightly different read method
    aht_in.getEvent(&humidity_in_event, &temp_in_event);
    aht_out.getEvent(&humidity_out_event, &temp_out_event);
    
    // Update global variables
    humidityEntrance = humidity_in_event.relative_humidity;
    tempEntrance = temp_in_event.temperature;
    
    humidityOutput = humidity_out_event.relative_humidity;
    tempOutput = temp_out_event.temperature;
    
    // Debugging print
    // Serial.printf("Hum IN: %.1f%% | Hum OUT: %.1f%%\n", humidityEntrance, humidityOutput);

    vTaskDelay(1000 / portTICK_PERIOD_MS); // 1 Hz read rate
  }
}

void initSensorTask() {
  xTaskCreate(sensorTask, "SensorTask", 4096, NULL, 1, NULL);
}