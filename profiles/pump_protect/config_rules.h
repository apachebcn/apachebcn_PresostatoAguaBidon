// ============================================================
//  PERFIL PUMP PROTECT (config_rules.h)
//  Reglas de decisión de la SALIDA (relé) y de la ALARMA (led + buzzer).
//  Edita SOLO el texto que hay entre las comillas.
// ============================================================
#pragma once

// Una regla por línea. Formato:
//
//   CONDICION : SALIDA
//
//   CONDICION = lista de "NOMBRE=valor" unidos con & (Y) o con | (OR)
//               (valor = 0 o 1). Ejemplo: "E1=1|E2=1:1" = E1 mojado O E2 mojado.
//   SALIDA    = 1 (relé ON) o 0 (relé OFF)
//
// Nombres disponibles:
//   E1 = electrodo 1  (1 = mojado, 0 = seco)
//   E2 = electrodo 2  (1 = mojado, 0 = seco)
//   L  = nivel        (1 = mojado, 0 = seco)
//   V  = VALOR resultante ACTUAL (1 o 0). Sirve para comparar los sensores
//        con el valor que acaba de decidir el sistema (realimentación).
//
// Las reglas se evalúan EN ORDEN y gana la PRIMERA que encaje.
// Las líneas que empiezan por # o ; se ignoran (comentarios).

const char RULES_FOR_RELAY[] =
    "E1=1:1\n";   // E1 mojado -> relé ON  (el resto de hardware no se usa)

// Reglas de la ALARMA (led + buzzer). Mismas entradas E1, E2, L y V
// (aquí V = valor resultante actual de la alarma).
const char RULES_FOR_ALARM[] =
    "E1=0:1\n";  // E1 seco -> alarma ON
