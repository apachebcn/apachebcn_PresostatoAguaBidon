// ============================================================
//  CONFIG_GENERAL.H
//  Configuración GENERAL del sistema (compartida por todos los perfiles).
//  Lo específico de cada aplicación vive en profiles/<perfil>/config_general.h.
// ============================================================
#pragma once

// ============================================================
// PERFIL: cambia esta línea para elegir otro perfil.
// Cada perfil vive en "profiles/<carpeta>/" y agrupa, vía profile.h:
//   config_general.h, config_electrode_1.h, config_electrode_2.h,
//   config_level.h y config_rules.h.
// Incluye SOLO UNO a la vez.
// ============================================================
#include "profiles/drum_filling/profile.h"

// Habilita/deshabilita la impresión por Serial
#define PRINT_SERIAL 1

// Habilita/deshabilita el procesamiento de los jumpers (forzar relé ON/OFF, brillo de leds).
// Valor por defecto; cada perfil puede sobreescribirlo definiéndolo antes en su
// profiles/<perfil>/config_general.h (p. ej. #define PROCESS_JUMPERS 0 si no usa jumpers).
#define PROCESS_JUMPERS 1

// Valores analógicos para el brillo del LED
#define LED_ANALOG_VALUE_BRIGHT_HIGH 200
#define LED_ANALOG_VALUE_BRIGHT_LOW 252

// Estados del LED
#define LED_ON 1
#define LED_OFF 0

// Verdadero cuando toca ejecutar la decisión principal del bucle
// (cada DELAY_IN_LOOP ms, o la primera vez). Gobierna el bloque TIME_LOOP del sketch.
// DELAY_IN_LOOP y DELAY_IN_ALARM vienen del perfil (profiles/<perfil>/config_general.h).
#define TIME_LOOP (lastProcessLoopTime == 0 || (millis() - lastProcessLoopTime) >= DELAY_IN_LOOP)

// Verdadero cuando toca emitir el pitido de alarma normal (cada DELAY_IN_ALARM ms).
#define TIME_ALARM (lastProcessAlarmTime == 0 || (millis() - lastProcessAlarmTime) >= DELAY_IN_ALARM)

// Verdadero cuando toca emitir la sirena de la alarma de fuga (cada DELAY_IN_ALARM ms).
#define TIME_LEAK_ALARM (lastProcessLeakAlarmTime == 0 || (millis() - lastProcessLeakAlarmTime) >= DELAY_IN_ALARM)
