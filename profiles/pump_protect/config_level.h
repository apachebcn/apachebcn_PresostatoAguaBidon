// ============================================================
//  CONFIG_LEVEL.H
//  Configuración del sensor de NIVEL (presostato).
// ============================================================
#pragma once

// Habilita/deshabilita el procesamiento del sensor de nivel
#define PROCESS_LEVEL 0

// Tiempo que se deja el pulso encendido antes de leer el nivel (en microsegundos).
// Subirlo (p. ej. a 200-500) da más tiempo al sensor para estabilizarse -> más sensible.
#define LEVEL_READS_TIME_MICROSECONDS 100

// Lógica de "mojado" del nivel:
// El código compara (levelValue == LEVEL_WATER_TRUE_CONTACT_WITH_HARDWARE_GET_VALUE) para saber si el nivel toca agua.
//   1 = "tocando agua" se representa con valor 1 (lógica positiva, la habitual).
//   0 = se invertiría la lógica (tocando agua = 0). Mantener en 1 salvo que cambies el hardware.
#define LEVEL_WATER_TRUE_CONTACT_WITH_HARDWARE_GET_VALUE 1

// Alarma por nivel: 1 = suena la alarma cuando el nivel toca agua, 0 = silencio.
// Es independiente de PROCESS_LEVEL: puedes procesar el nivel sin que alarme, o al revés.
#define LEVEL_ALARM_ENABLE 1

// Testigo LED del nivel:
//   1 = LED encendido cuando el nivel TOCA AGUA (mojado).
//   0 = LED encendido cuando el nivel está SECO (testigo invertido).
#define LEVEL_LED_ON_WHEN_WATER_CONTACT 1
