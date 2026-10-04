// ============================================================
//  CONFIG_ELECTRODE_1.H
//  Configuración del ELECTRODO 1 (todo lo suyo en un solo sitio).
// ============================================================
#pragma once

// Habilita/deshabilita el procesamiento del electrodo 1
#define PROCESS_ELECTRODE_1 1

// Depuración: imprime el valor analógico en bruto de E1 en cada lectura
// (útil para recalibrar el umbral, p. ej. tras cambiar resistencias).
// 1 = activo, 0 = silencio
#define PRINT_SERIAL_ELECTRODE_1_DEBUG_RAW 0

// Retardo entre encender el pulso y ARRANCAR la conversión del ADC de E1 (en microsegundos).
// NO es la duración del pulso: el pulso se corta en cuanto el ADC ha capturado la muestra
// (ver analogReadShortPulse() y ADC_SAMPLE_HOLD_MICROSECONDS), así que la corriente por el
// agua dura aproximadamente (este valor + ADC_SAMPLE_HOLD_MICROSECONDS) ~= 25-30 us.
// Subirlo (p. ej. a 200-500) da más tiempo al transistor para conducir cuando los electrodos
// están más separados (más resistencia del agua) -> más sensible, pero también alarga la
// corriente por el agua (más electrólisis). Con las vainas de acero inox, 1 us basta.
#define ELECTRODE_1_READS_TIME_MICROSECONDS 1

// Polaridad de "mojado" de E1:
//   1 = mojado se detecta con valor analógico BAJO (value < umbral). Es lo habitual:
//       el agua cierra el circuito y tira la tensión leída hacia abajo (mojado ~0, seco ~1023).
//   0 = mojado se detecta con valor analógico ALTO (value > umbral), si cambias el hardware
//       para que el agua suba la tensión en vez de bajarla.
#define ELECTRODE_1_WATER_TRUE_CONTACT_WITH_HARDWARE_GET_VALUE 1

// Sensibilidad de detección de E1.
// Valor analógico (0-1023) por debajo del cual se considera que el agua toca el electrodo.
// Cuanto MÁS ALTO, MÁS sensible (necesario si separas más los cables del electrodo en el
// agua: la resistencia del agua crece con la distancia y el transistor conduce menos).
// Cuanto más bajo, más inmune a falsos positivos (humedad, espuma, película de agua).
// CALIBRADO: seco ~1023, mojado ~0 con las vainas de acero -> 100 va con mucho margen.
#define ELECTRODE_1_CONTACT_ANALOG_THRESHOLD 100

// Alarma: 1 = suena la alarma cuando E1 toca agua, 0 = silencio.
#define ELECTRODE_1_ALARM_ENABLE 1

// Testigo LED del electrodo 1:
//   1 = LED encendido cuando el electrodo TOCA AGUA (mojado).
//   0 = LED encendido cuando el electrodo está SECO (testigo invertido).
#define ELECTRODE_1_LED_ON_WHEN_WATER_CONTACT 1
