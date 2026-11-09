#include "encoder_menu.h"
#include "display.h" 
#include "display/FreeSerifItalic12pt7b.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

extern Adafruit_ST7789 tft;

// Variables de estado
volatile bool inMenu = false;
volatile bool alarmaActivada = false; // Inicia desactivada por defecto
int selectedOption = 0;
const int NUM_OPTIONS = 4;

int lastClk = HIGH;
unsigned long lastEncoderBtnTime = 0;
unsigned long lastSelectBtnTime = 0;

void setupEncoder() {
  pinMode(ENCODER_CLK, INPUT_PULLUP);
  pinMode(ENCODER_DT, INPUT_PULLUP);
  pinMode(ENCODER_SW, INPUT_PULLUP);
  pinMode(BTN_SELECT, INPUT_PULLUP); // Pulsador para aceptar
  pinMode(BTN_EXTRA, INPUT_PULLUP);  // Pulsador extra sin uso aún
  lastClk = digitalRead(ENCODER_CLK);
}

void drawMenuScreen() {
  tft.fillScreen(0x0000); 
  tft.setFont(nullptr); 
  tft.setTextSize(2);
  tft.setTextColor(0xFFFF); 
  tft.setCursor(20, 20);
  tft.println("--- MENU PRINCIPAL ---");

  tft.setFont(&FreeSerifItalic12pt7b);
  tft.setTextSize(1);
  
  for (int i = 0; i < NUM_OPTIONS; i++) {
    int yPos = 80 + (i * 40); 
    
    if (i == selectedOption) {
      tft.fillRect(10, yPos - 20, 300, 30, 0x07E0); 
      tft.setTextColor(0x0000); 
      tft.setCursor(20, yPos);
      tft.print("> ");
    } else {
      tft.setTextColor(0xFFFF); 
      tft.setCursor(20, yPos);
      tft.print("   ");
    }
    
    // Nombres dinámicos de las opciones
    if (i == 0) {
      if (alarmaActivada) {
        tft.print("1. Desactivar Alarma");
      } else {
        tft.print("1. Activar Alarma");
      }
    } else if (i == 1) {
      tft.print("2. Personalizacion");
    } else if (i == 2) {
      tft.print("3. Mas Informacion");
    } else if (i == 3) {
      tft.print("4. Salir del Menu");
    }
  }
}

void processEncoder() {
  // 1. Botón del Encoder (Exclusivo para Abrir/Cerrar menú en cualquier momento)
  if (digitalRead(ENCODER_SW) == LOW) {
    if (millis() - lastEncoderBtnTime > 300) { 
      if (!inMenu) {
        inMenu = true;
        selectedOption = 0;
        drawMenuScreen();
      } else {
        inMenu = false;
        drawStaticBase();      
        updateDynamicValues(); 
      }
      lastEncoderBtnTime = millis();
    }
  }

  // 2. Botón SELECT (Exclusivo para aceptar opción cuando estás en el menú)
  if (inMenu && digitalRead(BTN_SELECT) == LOW) {
    if (millis() - lastSelectBtnTime > 300) {
      
      if (selectedOption == 0) { 
        // Cambiar el estado de la alarma y redibujar el texto inmediatamente
        alarmaActivada = !alarmaActivada;
        drawMenuScreen(); 
      } 
      else if (selectedOption == 3) { 
        // Salir del menú usando la opción 4
        inMenu = false;
        drawStaticBase();      
        updateDynamicValues(); 
      } 
      else { 
        // Opciones en desarrollo
        tft.fillScreen(0x0000);
        tft.setCursor(20, 120);
        tft.setTextColor(0xFFFF);
        tft.setTextSize(2);
        tft.print("Opcion en desarrollo");
        delay(1500);
        drawMenuScreen(); 
      }
      lastSelectBtnTime = millis();
    }
  }

  // 3. Lectura de rotación (Giro)
  if (inMenu) {
    int currentClk = digitalRead(ENCODER_CLK);
    if (currentClk != lastClk && currentClk == LOW) {
      int dtValue = digitalRead(ENCODER_DT);
      if (dtValue == HIGH) { 
        selectedOption++;
        if (selectedOption >= NUM_OPTIONS) selectedOption = 0; 
      } else {               
        selectedOption--;
        if (selectedOption < 0) selectedOption = NUM_OPTIONS - 1; 
      }
      drawMenuScreen();
    }
    lastClk = currentClk;
  }
}