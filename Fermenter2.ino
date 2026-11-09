
#include <Arduino.h>
#include "sensors.h"
#include "display.h"  
#include "encoder_menu.h"

#define TFT_BL   4


void setup() {
  Serial.begin(115200);
  Serial.println("Iniciando sistema...");

  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  initDisplayTask();

  initSensors();
  initDisplayTask();
  initSensorTask();

  setupEncoder();
}

void loop() {
  // 🔹 Aquí puedes poner pruebas de lógica principal
  // Por ahora solo incrementamos un contador cada segundo
  static unsigned long lastUpdate = 0;
  static int counter = 0;
  processEncoder();

  /*unsigned long now = millis();
  if (now - lastUpdate >= 10000) {
    if (counter%2 == 0){flameIcon.active = true; fanIcon.active = true; humidifierIcon.active = true;}
    else{ flameIcon.active = false;fanIcon.active = false; humidifierIcon.active = false;}
    counter++;
    Serial.print("Loop principal activo, counter = ");
    Serial.println(counter);
    lastUpdate = now;
  }*/

}
