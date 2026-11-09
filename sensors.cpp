// sensors.cpp
#include "sensors.h"
#include <Wire.h>
#include <Adafruit_SHT31.h>

#define SHT31_ADDR_1 0x44
#define SHT31_ADDR_2 0x45

Adafruit_SHT31 sht1 = Adafruit_SHT31();
Adafruit_SHT31 sht2 = Adafruit_SHT31();

// Variables compartidas, leídas por displayTask y actuadores
volatile float currentTemperature = 0.0;
volatile float currentHumidity = 0.0;
volatile bool sensorMismatchError = false;

void initSensors() {
  Wire.begin(21, 22); // SDA, SCL
  delay(100);         

  if (!sht1.begin(SHT31_ADDR_1)) {
    Serial.println("SHT31 #1 (0x44) no encontrado");
  } else {
    Serial.println("SHT31 #1 inicializado");
  }

  if (!sht2.begin(SHT31_ADDR_2)) {
    Serial.println("SHT31 #2 (0x45) no encontrado");
  } else {
    Serial.println("SHT31 #2 inicializado");
  }
}

void sensorTask(void *pvParameters) {
  for (;;) {
    float t1 = sht1.readTemperature();
    float h1 = sht1.readHumidity();
    float t2 = sht2.readTemperature();
    float h2 = sht2.readHumidity();

    // 1. Verificar lecturas fallidas a nivel de bus I2C (NaN)
    if (isnan(t1) || isnan(t2) || isnan(h1) || isnan(h2)) {
      sensorMismatchError = true;
      Serial.println("ALERTA: Falla de lectura (NaN) en sensores SHT31.");
    } else {
      // 2. Verificar redundancia y límites extremos
      bool difExcesiva = abs(t1 - t2) > 10.0;
      bool tempExtrema = (t1 < 0.0 || t1 > 110.0 || t2 < 0.0 || t2 > 110.0);

      if (difExcesiva || tempExtrema) {
        sensorMismatchError = true;
        Serial.print("ERROR SENSOR! T1="); Serial.print(t1);
        Serial.print(" T2="); Serial.println(t2);
      } else {
        // 3. Todo correcto: desactivar error y actualizar valores globales
        sensorMismatchError = false;
        currentTemperature = (t1 + t2) / 2.0;
        currentHumidity = (h1 + h2) / 2.0;
      }
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS); // Tasa de lectura de 1 Hz
  }
}

void initSensorTask() {
  xTaskCreate(sensorTask, "SensorTask", 4096, NULL, 1, NULL);
}