// ============================================================
//  CONFIG_GENERAL.H (PERFIL PUMP PROTECT)
//  Configuración general ESPECÍFICA de este perfil.
//  Se incluye desde profile.h.
// ============================================================
#pragma once


// Retardo principal del bucle, el que toma las decisiones sobre el relé (en milisegundos)
#define DELAY_IN_LOOP 1000

// Retardo para alarmas (en milisegundos)
#define DELAY_IN_ALARM 4000

// Tiempo (en microsegundos) que se mantiene el pulso de los electrodos hasta que el ADC ha
// capturado la muestra, momento en el que ya se puede cortar (ver analogReadShortPulse()).
// El "sample & hold" es un circuito INTERNO del ATmega (interruptor + condensador diminuto
// dentro del chip; no hay ningún condensador en la placa). Se necesitan 2,5 ciclos de ADC:
// 1,5 para el sample & hold + hasta 1 de sincronización del arranque de la conversión.
// Con el prescaler por defecto (/128) un ciclo de ADC dura 128/F_CPU segundos, así que:
//   Arduino Pro Mini 5 V   (16 MHz) -> 20 us + margen = 25 us
//   Arduino Pro Mini 3,3 V ( 8 MHz) -> 40 us + margen = 45 us
// Se calcula solo a partir de F_CPU: no hay que tocar nada al cambiar de versión.
#define ADC_SAMPLE_HOLD_MICROSECONDS ((unsigned int)(320000000UL / F_CPU) + 5)

// Configuración de la alarma de fuga (relé activado demasiado tiempo)
#define LEAK_ALARM_ENABLE 0  // 1 = activa la alarma de fuga, 0 = desactivada
#define LEAK_ALARM_TIME_SECONDS 0 // Tiempo máximo (segundos) con el relé activado

// Estados del relé
#define RELAY_ON 0
#define RELAY_OFF 1

// Estado del relé cuando se dispara la alarma de fuga (LEAK_ALARM_ENABLE=1).
//  RELAY_OFF = relé abierto al dispararse (lo habitual: cortar el agua/parar la salida).
//  RELAY_ON  = relé cerrado al dispararse (mantener la salida activa).
#define LEAK_ALARM_RELAY_STATE RELAY_OFF
