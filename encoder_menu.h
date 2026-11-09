#ifndef ENCODER_MENU_H
#define ENCODER_MENU_H

#include <Arduino.h>

// Pines del encoder
#define ENCODER_CLK 14
#define ENCODER_DT  15
#define ENCODER_SW  13

// Pines de los nuevos pulsadores
#define BTN_SELECT  26 
#define BTN_EXTRA   25 

// Variables compartidas
extern volatile bool inMenu;
extern volatile bool alarmaActivada; // Nuevo estado de alarma

// Declaración de las funciones
void setupEncoder();
void drawMenuScreen();
void processEncoder();

#endif