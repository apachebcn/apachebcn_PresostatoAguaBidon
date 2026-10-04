// ============================================================
//  PERFIL SAMPLE (config_rules.h)
//  Perfil de EJEMPLO. Para crear un perfil nuevo:
//    1) Copia esta carpeta entera y renómbrala (p. ej. profiles/mi_perfil)
//    2) Edita RULES_FOR_RELAY, RULES_FOR_ALARM y los config_*.h de la carpeta
//    3) Cambia el #include en config_general.h para elegirlo
//
//  IMPORTANTE: cada perfil define RULES_FOR_RELAY y RULES_FOR_ALARM.
//  Solo se incluye UN perfil a la vez.
//
//  Una regla por línea: CONDICION : SALIDA
//    CONDICION = NOMBRE=0/1 unidos con & (Y) o con | (OR)
//    SALIDA    = 1 (relé ON) o 0 (relé OFF)
//    Nombres: E1, E2 (electrodos), L (nivel), V (valor resultante actual)
//    Primera regla que encaje gana.
//    Líneas que empiezan por # o ; son comentarios.
// ============================================================
#pragma once

const char RULES_FOR_RELAY[] =
    "E1=1:1\n"    // E1 mojado      -> relé ON
    "E2=1:0\n"    // E2 mojado      -> relé OFF
    "L=0:1\n"     // nivel seco     -> relé ON
    "L=1&V=0:0\n"; // nivel mojado y valor actual 0 -> relé OFF

// Reglas de la ALARMA (led + buzzer). Mismas entradas E1, E2, L y V
// (aquí V = valor resultante actual de la alarma).
const char RULES_FOR_ALARM[] =
    "E1=1|E2=1|L=1:1\n";  // cualquier sensor mojado -> alarma ON
