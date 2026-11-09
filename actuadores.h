#ifndef ACTUADORES_H
#define ACTUADORES_H

#define PIN_POWER_MAIN 32
#define PIN_HEAT_CTRL  33
#define PIN_HUMIDIFIER  27

void setupActuadores() {
  pinMode(PIN_POWER_MAIN, OUTPUT);
  pinMode(PIN_HEAT_CTRL, OUTPUT);
  pinMode(PIN_HUMIDIFIER, OUTPUT);

  // Empezamos con todo apagado por seguridad
  digitalWrite(PIN_POWER_MAIN, LOW);
  digitalWrite(PIN_HEAT_CTRL, LOW); 
  digitalWrite(PIN_HUMIDIFIER, LOW);
}

void controlarSistema(EstadoSistema &st) {
  // 1. Lógica de Poder General (Ejemplo: Solo si estamos en Modo > 0)
  if (st.modo > 0) {
    digitalWrite(PIN_POWER_MAIN, HIGH);
  } else {
    digitalWrite(PIN_POWER_MAIN, LOW);
  }

  // 2. Lógica de Calefacción (Tu "valor analógico de 1" es un HIGH digital)
  // Si la temperatura real es mayor a la deseada, deshabilitamos calor
  if (st.tempReal >= st.tempsObj[st.modo - 1]) {
    digitalWrite(PIN_HEAT_CTRL, HIGH); // "Deshabilitar"
  } else {
    digitalWrite(PIN_HEAT_CTRL, LOW);  // "Habilitar"
  }

  // 3. Lógica de Humidificador
  if (st.humReal < st.humsObj[st.modo - 1]) {
    digitalWrite(PIN_HUMIDIFIER, HIGH); // Encender
  } else {
    digitalWrite(PIN_HUMIDIFIER, LOW);  // Apagar
  }
}

#endif