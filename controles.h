#ifndef CONTROLES_H
#define CONTROLES_H

struct EstadoSistema {
  int modo = 1;              // 1-4 y Custom(5)
  bool editando = false;      // true: estado de edicion, false: no se puede editar
  bool editandoTemp = true;  // true: Temp, false: Hum
  float tempObj[5] = {20.0, 22.0, 25.0, 28.0, 30.0};
  float humObj[5] = {60.0, 65.0, 70.0, 75.0, 90.0};
  float tempReal = 25.4;     // Estos vendrán del SHT30 luego
  float humReal = 70.2;
  float potFiltrado = 0; // Para guardar el promedio del potenciometro

  // Variables de tiempo para el botón
  uint32_t tPresionado = 0;  //guardar el momento en que se oprimió el botón.
  bool botonPresionado = false;

};

void setupHardware() {
  pinMode(25, INPUT_PULLUP); // Botón MODO
  pinMode(26, INPUT_PULLUP); // Botón SELECT
}

void procesarEntradas(EstadoSistema &st) {
  int lecturaModo = digitalRead(25); // Botón MODO
  uint32_t ahora = millis();

  // 1. DETECTAR CUÁNDO SE PRESIONA
  if (lecturaModo == LOW && !st.botonPresionado) {
    st.tPresionado = ahora;
    st.botonPresionado = true;
  }

  // 2. MIENTRAS ESTÁ PRESIONADO: ¿Pasaron 3 segundos?
  if (lecturaModo == LOW && st.botonPresionado) {
    if (!st.editando && (ahora - st.tPresionado >= 2000)) {
      st.editando = true; // ENTRAR A EDICIÓN
      Serial.println("Modo Edición: ACTIVADO");
    }
  }

  // 3. DETECTAR CUÁNDO SE SUELTA
  if (lecturaModo == HIGH && st.botonPresionado) {
    uint32_t duracion = ahora - st.tPresionado;
    st.botonPresionado = false;

    if (duracion < 500) { // Fue un click corto
      if (st.editando) {
        st.editando = false; // SALIR DE EDICIÓN (Guardar)
        Serial.println("Modo Edición: GUARDADO");
      } else {
        st.modo = (st.modo % 5) + 1; // CAMBIAR DE MODO (1 al 5)
      }
    }
  }
  
  // Botón SELECT (GPIO 26) - Solo funciona si estamos editando
  if (st.editando && digitalRead(26) == LOW) {
    st.editandoTemp = !st.editandoTemp;
    delay(200); 
  }

  int rawPot = analogRead(34);

  // Filtro: 90% valor anterior + 10% valor nuevo
  // Esto elimina el parpadeo rápido
  st.potFiltrado = (st.potFiltrado * 0.9) + (rawPot * 0.1);

  // LÓGICA DEL POTENCIÓMETRO (Solo si editando == true)
  if (st.editando) {
  int potActual = (int)st.potFiltrado;
  static int potAnterior = 0;

    // Solo si el potenciómetro se movió más de 10 unidades (de 4095)
    if (abs(potActual - potAnterior) > 10) { 
      int idx = st.modo - 1;

      if (st.editandoTemp) {
        st.tempObj[idx] = map(potActual, 0, 4095, 15, 35);
      } else {
        st.humObj[idx] = map(potActual, 0, 4095, 60, 100);
      }
      potAnterior = potActual; // Guardamos la nueva posición estable
    }
  }
} // Aquí cierra la función procesarEntradas
#endif