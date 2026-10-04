#include <Arduino.h>
#include <string.h>

#include "config_general.h"
#include "config_pins.h"
#include "functions.h"

// ===== Motor de reglas =====
// Convierte las reglas de texto del perfil (RULES_FOR_RELAY y RULES_FOR_ALARM)
// en tablas que se consultan en cada decisión. Cada conjunto se parsea UNA sola
// vez (la primera vez que se necesita), así que el coste en cada decisión es mínimo.

#define RULES_MAX_INPUTS 8      // Nº máximo de entradas distintas (E1, E2, L, V, ...)
#define RULES_MAX_RULES 32      // Nº máximo de reglas (cada alternativa | ocupa una)
#define RULES_MAX_NAME_LEN 8    // Longitud máxima del nombre de una entrada
#define RULES_MAX_ORS 8         // Máximo de alternativas (|) dentro de una regla

struct ParsedRule {
	uint8_t mask;     // Qué entradas mira la regla
	uint8_t pattern;  // Qué valores tienen que tener esas entradas
	uint8_t out;      // Resultado: 1 = ON, 0 = OFF
};

// Un conjunto de reglas (relé o alarma) con su propia tabla y sus propias entradas.
struct RuleSet {
	char names[RULES_MAX_INPUTS][RULES_MAX_NAME_LEN + 1];
	uint8_t inputCount;
	ParsedRule rules[RULES_MAX_RULES];
	uint8_t ruleCount;
	bool parsed;
};

static RuleSet relayRuleSet;
static RuleSet alarmRuleSet;

// Devuelve el bit asignado a un nombre de entrada (y lo crea si es nuevo).
// Devuelve 0 si ya no caben más entradas.
static uint8_t rulesGetOrAddInput(RuleSet& set, const char* name)
{
	for (uint8_t i = 0; i < set.inputCount; i++)
	{
		if (strcmp(set.names[i], name) == 0)
			return (uint8_t)(1u << i);
	}
	if (set.inputCount >= RULES_MAX_INPUTS) return 0;

	strncpy(set.names[set.inputCount], name, RULES_MAX_NAME_LEN);
	set.names[set.inputCount][RULES_MAX_NAME_LEN] = '\0';
	uint8_t bit = (uint8_t)(1u << set.inputCount);
	set.inputCount++;
	return bit;
}

// Valor actual de una entrada según su nombre.
// forAlarm=true -> el "V" (valor resultante actual) es el de la alarma;
// forAlarm=false -> es el del relé.
static bool rulesInputValue(const char* name, bool forAlarm)
{
	if (strcmp(name, "E1") == 0) return electrode1Value;  // electrodo 1 mojado
	if (strcmp(name, "E2") == 0) return electrode2Value;  // electrodo 2 mojado
	if (strcmp(name, "L") == 0) return levelValue;        // nivel mojado
	if (strcmp(name, "V") == 0) return forAlarm ? alarm : relay_on;  // valor resultante actual
	return false;  // nombre desconocido -> se lee como 0
}

static void rulesPrint(RuleSet& set)
{
	if (!PRINT_SERIAL) return;

	Serial.print("REGLAS CARGADAS: ");
	Serial.println(set.ruleCount);
	Serial.print("ENTRADAS DETECTADAS:");
	for (uint8_t i = 0; i < set.inputCount; i++)
	{
		Serial.print(' ');
		Serial.print(set.names[i]);
	}
	Serial.println();

	for (uint8_t r = 0; r < set.ruleCount; r++)
	{
		Serial.print("  R");
		Serial.print(r + 1);
		Serial.print(": ");
		bool first = true;
		for (uint8_t i = 0; i < set.inputCount; i++)
		{
			uint8_t bit = (uint8_t)(1u << i);
			if (set.rules[r].mask & bit)
			{
				if (!first) Serial.print(" & ");
				first = false;
				Serial.print(set.names[i]);
				Serial.print('=');
				Serial.print((set.rules[r].pattern & bit) ? '1' : '0');
			}
		}

		Serial.print(" : ");
		Serial.println(set.rules[r].out);
	}
}

// Lee las reglas de un conjunto y deduce la estructura (entradas + tabla).
static void rulesParse(RuleSet& set, const char* text)
{
	set.inputCount = 0;
	set.ruleCount = 0;

	const char* p = text;
	while (*p)
	{
		// Salta espacios y líneas vacías
		while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
		if (!*p) break;

		// Línea de comentario
		if (*p == '#' || *p == ';')
		{
			while (*p && *p != '\n') p++;
			continue;
		}

		uint8_t masks[RULES_MAX_ORS + 1];
		uint8_t patterns[RULES_MAX_ORS + 1];
		uint8_t conjCount = 0;
		uint8_t mask = 0;
		uint8_t pattern = 0;
		bool ok = true;

		// Términos NOMBRE=valor unidos con & ; alternativas separadas con |
		for (;;)
		{
			char name[RULES_MAX_NAME_LEN + 1];
			uint8_t n = 0;
			while (*p && *p != '=' && *p != ':' && *p != '&' && *p != '|' && *p != ' ' && *p != '\t')
			{
				if (n < RULES_MAX_NAME_LEN) name[n++] = *p;
				p++;
			}
			name[n] = '\0';

			while (*p == ' ' || *p == '\t') p++;
			if (*p != '=') { ok = false; break; }
			p++;
			while (*p == ' ' || *p == '\t') p++;

			char value = *p;
			if (value != '0' && value != '1') { ok = false; break; }
			p++;

			uint8_t bit = rulesGetOrAddInput(set, name);
			if (bit == 0) { ok = false; break; }

			mask |= bit;
			if (value == '1') pattern |= bit;

			while (*p == ' ' || *p == '\t') p++;
			if (*p == '&')
			{
				p++;
				while (*p == ' ' || *p == '\t') p++;
				continue;
			}

			// Fin de una conjunción: la guardamos
			if (conjCount <= RULES_MAX_ORS)
			{
				masks[conjCount] = mask;
				patterns[conjCount] = pattern;
				conjCount++;
			}

			if (*p == '|')  // Nueva alternativa dentro de la misma regla
			{
				p++;
				while (*p == ' ' || *p == '\t') p++;
				mask = 0;
				pattern = 0;
				continue;
			}
			break;
		}

		while (*p == ' ' || *p == '\t') p++;
		if (*p != ':') ok = false;
		else p++;

		while (*p == ' ' || *p == '\t') p++;
		uint8_t out = 0;
		if (*p == '1') out = 1;
		else if (*p == '0') out = 0;
		else ok = false;

		// Avanza hasta el final de la línea
		while (*p && *p != '\n') p++;

		if (ok)
		{
			for (uint8_t i = 0; i < conjCount && set.ruleCount < RULES_MAX_RULES; i++)
			{
				set.rules[set.ruleCount].mask = masks[i];
				set.rules[set.ruleCount].pattern = patterns[i];
				set.rules[set.ruleCount].out = out;
				set.ruleCount++;
			}
		}
	}

	set.parsed = true;
	rulesPrint(set);
}

// Evalúa un conjunto de reglas. Primera regla que encaje gana.
static bool rulesApply(RuleSet& set, const char* text, bool forAlarm)
{
	if (!set.parsed) rulesParse(set, text);

	// Construye el estado actual de las entradas en un byte de bits
	uint8_t state = 0;
	for (uint8_t i = 0; i < set.inputCount; i++)
	{
		if (rulesInputValue(set.names[i], forAlarm)) state |= (uint8_t)(1u << i);
	}

	for (uint8_t i = 0; i < set.ruleCount; i++)
	{
		if ((state & set.rules[i].mask) == set.rules[i].pattern)
			return set.rules[i].out != 0;
	}
	return false;  // Sin reglas o sin coincidencia
}

// Decide el relé (salida) según RULES_FOR_RELAY.
bool relayRulesApply()
{
	return rulesApply(relayRuleSet, RULES_FOR_RELAY, false);
}

// Decide la alarma (led + buzzer) según RULES_FOR_ALARM.
bool alarmRulesApply()
{
	return rulesApply(alarmRuleSet, RULES_FOR_ALARM, true);
}

// ===== Estado de los LEDs =====
// Se define aquí porque solo las funciones de este módulo lo usan.
struct LedStatus {  // Estructura para manejar el estado de los leds
	unsigned int pin;
	bool value;
};
LedStatus leds[] = {  // Array de estructuras para manejar el estado de los leds
	{PIN_SCAN_LED, false},
	{PIN_LEVEL_LED, false},
	{PIN_ELECTRODE1_LED, false},
	{PIN_ELECTRODE2_LED, false},
	{PIN_RELAY_LED, false}
};
const size_t NUM_LEDS = sizeof(leds) / sizeof(leds[0]);  // Número de leds


void serial_println()
{
	if (PRINT_SERIAL)
	{
		Serial.println();
	}
}

void serial_println(String message)
{
	if (PRINT_SERIAL)
	{
		Serial.println(message);
	}
}

void updateBrightLeds()
{
	// Control de brillo de los leds
	// Si el jumper de leds alto está puesto, brillo alto
	if (PROCESS_JUMPERS && valueSwitchLedsHigh)
	{
		analogWrite(PIN_GND_PWM_LED, LED_ANALOG_VALUE_BRIGHT_HIGH);
	}
	else
	{
		analogWrite(PIN_GND_PWM_LED, LED_ANALOG_VALUE_BRIGHT_LOW);
	}
}

void updateLeds()
{
	for (size_t numLed = 0; numLed < NUM_LEDS; ++numLed)
	{
		bool valueLed = leds[numLed].value;
		int pinLed = leds[numLed].pin;

		if (intermitentEnable and pinLed != PIN_SCAN_LED)
		{
			// Lógica para leds intermitentes
			if (valueLed)
			{
				if (intermitentStepOn)
				{
					digitalWrite(pinLed, LED_ON);
					updateBrightLeds();
				}
				else
				{
					digitalWrite(pinLed, LED_OFF);
					updateBrightLeds();
				}
			}
			else
			{
				digitalWrite(pinLed, LED_OFF);
				updateBrightLeds();
			}
		}
		else
		{
			// Lógica para leds normales
			digitalWrite(pinLed, valueLed);
			updateBrightLeds();
		}
	}
}

void ledOn(unsigned int pin_led)
{
	// Enciende el bit del led
	for (size_t numLed = 0; numLed < NUM_LEDS; ++numLed)
	{
		if (leds[numLed].pin == pin_led)
		{
			leds[numLed].value = LED_ON;
			break;
		}
	}
}

void ledOff(unsigned int pin_led)
{
	// Apaga el bit del led
	for (size_t numLed = 0; numLed < NUM_LEDS; ++numLed)
	{
		if (leds[numLed].pin == pin_led)
		{
			leds[numLed].value = LED_OFF;
			break;
		}
	}
}

void ledsAllOn()
{
	// Enciende todos los leds
	ledOn(PIN_SCAN_LED);
	ledOn(PIN_LEVEL_LED);
	ledOn(PIN_ELECTRODE1_LED);
	ledOn(PIN_ELECTRODE2_LED);
	ledOn(PIN_RELAY_LED);
}

void ledsAllOff()
{
	// Apaga todos los leds
	ledOff(PIN_SCAN_LED);
	ledOff(PIN_LEVEL_LED);
	ledOff(PIN_ELECTRODE1_LED);
	ledOff(PIN_ELECTRODE2_LED);
	ledOff(PIN_RELAY_LED);
}

void ledsTest()
{
	// Prueba de leds
	for (size_t numLed = 0; numLed < NUM_LEDS; ++numLed)
	{
		digitalWrite(leds[numLed].pin, LED_ON);
	}
	int i = 0;
	for(i=0; i < 255; i++)
	{
		analogWrite(PIN_GND_PWM_LED, i);
		delay(1);
	}
	for (size_t numLed = 0; numLed < NUM_LEDS; ++numLed)
	{
		digitalWrite(leds[numLed].pin, LED_OFF);
	}
}

bool getRelayActionOn(bool switchForceRelayOn, bool switchForceRelayOff, bool levelValue, bool electrode1Value, bool electrode2Value)
{
	// Determina si se debe ACTIVAR el relé (salida ON) o DESACTIVARLO (salida OFF).
	//
	// La seguridad va FUERA de las reglas (no se puede cambiar desde profiles/):
	//   1) Alarma de fuga   -> estado definido por el perfil (LEAK_ALARM_RELAY_STATE)
	//   2) Jumpers manuales -> forzar ON / forzar OFF
	// Después decide el motor de reglas (profiles/), que recibe E1, E2, L y V.

	if (LEAK_ALARM_ENABLE && leakAlarm) return (LEAK_ALARM_RELAY_STATE == RELAY_ON);  // Relé al estado del perfil
	if (PROCESS_JUMPERS && switchForceRelayOn) return true;
	if (PROCESS_JUMPERS && switchForceRelayOff) return false;

	return relayRulesApply();  // Decisión según las reglas del perfil (profiles/)
}

void refreshRelay(bool relay_on)
{
	// Actualiza el estado del relé
	if (relay_on)
	{
		digitalWrite(PIN_RELE, RELAY_ON);  // Relé On
		ledOn(PIN_RELAY_LED);  // Testigo ON del Relé en el led
	}
	else
	{
		digitalWrite(PIN_RELE, RELAY_OFF);  // Relé Off
		ledOff(PIN_RELAY_LED);  // Testigo OFF del Relé en el led
	}
}

void applyLeakAlarm()
{
	// Presentación de la alarma de fuga, gobernada por la variable leakAlarm
	// (igual que el beeper). Se llama cada ciclo del loop para garantizar que el
	// relé queda en el estado definido por el perfil (LEAK_ALARM_RELAY_STATE) y la
	// luz de alarma encendida aunque otros procesos hayan actualizado el relé o el
	// led del relé.
	if (LEAK_ALARM_ENABLE && leakAlarm)
	{
		digitalWrite(PIN_RELE, LEAK_ALARM_RELAY_STATE);  // Relé al estado decidido por el perfil
		// Alarma de fuga: todos los leds parpadean (excepto el de scan, que por
		// diseño no entra en el grupo de intermitentes y conserva su función).
		ledOn(PIN_LEVEL_LED);
		ledOn(PIN_ELECTRODE1_LED);
		ledOn(PIN_ELECTRODE2_LED);
		ledOn(PIN_RELAY_LED);
	}
}

void checkLeakAlarm()
{
	// Alarma de fuga: si el relé permanece activado más de LEAK_ALARM_TIME_SECONDS,
	// se bloquea el sistema (luz de alarma + beeper) hasta reiniciar el Arduino.
	if (!LEAK_ALARM_ENABLE) return;
	if (leakAlarm) return;  // Ya bloqueado

	// Excepción: si el jumper de forzar relé ON está cerrado (relé forzado a ON de forma
	// manual), el relé activado no se considera fuga.
	// Solo se vigila la activación automática (relé ON sin el jumper de forzar relé ON).
	bool relayOnAutomatic = relay_on && !(PROCESS_JUMPERS && valueSwitchForceRelayOn);

	if (relayOnAutomatic)
	{
		if (relayOnSince == 0)
		{
			relayOnSince = millis();  // Inicia el contador de tiempo de relé activado
		}
		else if ((millis() - relayOnSince) / 1000UL >= LEAK_ALARM_TIME_SECONDS)
		{
			leakAlarm = true;  // Bloquea el sistema
			relayOnSince = 0;

			serial_println();
			serial_println("ALARMA RIESGO DE FUGA: RELE ACTIVADO DURANTE MAS DE LEAK_ALARM_TIME_SECONDS");
			serial_println("SISTEMA BLOQUEADO. REINICIAR EL ARDUINO PARA DESBLOQUEAR.");

			relay_on = (LEAK_ALARM_RELAY_STATE == RELAY_ON);
			refreshRelay(relay_on);  // Relé al estado decidido por el perfil
			// La luz de alarma y el estado final del relé los aplica applyLeakAlarm() cada loop
		}
	}
	else
	{
		relayOnSince = 0;  // El relé está OFF o es llenado manual (jumper cerrado), se reinicia el contador
	}
}

bool isSwitchChanged(bool valueSwitch, bool lastState)
{
	// Detecta si hubo un cambio de estado en el pin
	if (valueSwitch != lastState) return true;
	return false;
}

void levelRead()
{
	digitalWrite(PIN_LEVEL_PULSE, HIGH);
	delayMicroseconds(LEVEL_READS_TIME_MICROSECONDS);
	// Estado EN BRUTO: interruptor cerrado = el pin 2 recibe el pulso (HIGH) -> 0;
	// interruptor abierto = el 4,7k a GND lo deja en LOW -> 1.
	levelRawValue = !digitalRead(PIN_LEVEL_READ_DATA);
	digitalWrite(PIN_LEVEL_PULSE, LOW);

	if (levelRawValue) levelSamplesOn++; else levelSamplesOff++;
}

bool levelReadApply()
{
	bool state = levelValue;
	if ((levelSamplesOn + levelSamplesOff) > 0)
	{
		state = levelSamplesOn >= levelSamplesOff;
	}

	levelSamplesOn = 0;
	levelSamplesOff = 0;

	#if PRINT_SERIAL_LEVEL_DEBUG_RAW
		// raw = última lectura en bruto del pin 2 (1 = LOW, interruptor abierto; 0 = con pulso,
		// interruptor cerrado). Compara el valor seco y mojado para confirmar la polaridad.
		serial_println("DEBUG LEVEL raw=" + String(levelRawValue) + " (LEVEL_WATER_TRUE_CONTACT_WITH_HARDWARE_GET_VALUE=" + String(LEVEL_WATER_TRUE_CONTACT_WITH_HARDWARE_GET_VALUE) + ")");
	#endif

	if (state == LEVEL_WATER_TRUE_CONTACT_WITH_HARDWARE_GET_VALUE)
		serial_println("RESULTADO -> LEVEL Tocando agua SÍ");
	else
		serial_println("RESULTADO -> LEVEL Tocando agua NO");

	return state;
}

unsigned int analogReadShortPulse(unsigned int pinPulse, unsigned int pinAnalog, unsigned int pulseTimeUs)
{
	// Lectura analógica con el pulso de alimentación MINIMIZADO (anti-electrólisis).
	//
	// Todo lo específico del micro (los registros del ADC del ATmega328P) está SOLO aquí.
	//
	// El ADC del ATmega328P tiene DENTRO del chip (no es un componente de la placa: no verás
	// ningún condensador en el esquemático) un circuito de "sample & hold": un interruptor y
	// un condensador minúsculo (picofaradios) conectados a la entrada analógica elegida. Al
	// arrancar la conversión, el interruptor cierra y el condensador se carga a la tensión del
	// pin durante ~1,5 ciclos de ADC; después el interruptor se abre y el ADC termina de
	// convertir comparando contra la tensión guardada en ese condensador.
	//
	// Por eso, una vez capturada la muestra, la entrada ya no influye: NO hace falta mantener
	// el pulso (y con él la corriente por el agua) durante los ~104 us que dura la conversión
	// completa; basta con ADC_SAMPLE_HOLD_MICROSECONDS.
	//
	// La precisión es la misma que con analogRead(): la muestra se captura en la misma ventana
	// de sample & hold. Solo se elimina la corriente "muerta" que circulaba por el agua
	// mientras el ADC terminaba de convertir.
	//
	// NO usa ningún pin nuevo: pinPulse y pinAnalog son los que ya usa cada electrodo.
	digitalWrite(pinPulse, HIGH);
	delayMicroseconds(pulseTimeUs);  // Deja conducir al transistor antes de muestrear

	// Convierte el pin analógico al canal del ADC (A0=14 ... A5=19 -> canal 0 ... 5),
	// exactamente igual que hace analogRead() por dentro. Así NO se declara ningún número
	// de canal a mano: el 2 o el 3 de un canal NO son pines, son entradas internas del
	// multiplexor del ADC correspondientes a A2 y A3.
	unsigned int channelADC = pinAnalog;
	if (channelADC >= 14) channelADC -= 14;

	// Selecciona el canal. Referencia AVcc (Vcc) y resultado alineado a la derecha, igual que
	// hace analogRead() por defecto (si se cambiase analogReference(), habría que ajustar
	// aquí REFS1/REFS0).
	ADMUX = _BV(REFS0) | (channelADC & 0x07);

	// Arranca la conversión: el sample & hold captura la tensión en los primeros
	// ~1,5 ciclos de ADC (más hasta 1 ciclo de sincronización del arranque).
	ADCSRA |= _BV(ADSC);

	// Espera lo justo para que el sample & hold haya capturado y CORTA EL PULSO: el resto de la
	// conversión ya no necesita corriente por el agua (aquí está el ahorro anti-electrólisis).
	delayMicroseconds(ADC_SAMPLE_HOLD_MICROSECONDS);
	digitalWrite(pinPulse, LOW);

	// Espera al final de la conversión (la tensión ya está guardada en el condensador interno
	// del ADC, no en el pin)
	while (bit_is_set(ADCSRA, ADSC)) { }

	// Hay que leer ADCL primero y ADCH después: leer ADCL bloquea el registro alto
	uint8_t lowByte = ADCL;
	uint8_t highByte = ADCH;
	return ((unsigned int)highByte << 8) | lowByte;
}

void electrode1Read()
{
	// Pulso minimizado: se corta en cuanto el ADC captura la muestra (anti-electrólisis)
	unsigned int value = analogReadShortPulse(PIN_ELECTRODES_PULSE, PIN_ELECTRODE1_READ_DATA, ELECTRODE_1_READS_TIME_MICROSECONDS);

	#if PRINT_SERIAL_ELECTRODE_1_DEBUG_RAW
		serial_println("DEBUG E1 raw=" + String(value) + " (umbral=" + String(ELECTRODE_1_CONTACT_ANALOG_THRESHOLD) + ")");
	#endif

	// true = mojado. ELECTRODE_1_WATER_TRUE_CONTACT_WITH_HARDWARE_GET_VALUE elige la polaridad:
	//   1 = mojado se detecta con valor analógico BAJO  (value < umbral, lo habitual)
	//   0 = mojado se detecta con valor analógico ALTO  (value > umbral, hardware invertido)
	bool state = ELECTRODE_1_WATER_TRUE_CONTACT_WITH_HARDWARE_GET_VALUE
		? (value < ELECTRODE_1_CONTACT_ANALOG_THRESHOLD)
		: (value > ELECTRODE_1_CONTACT_ANALOG_THRESHOLD);

	// ===== MODO INTERRUPTOR SIMPLE =====
	// La última muestra manda directamente: sin conteo ni media.
	electrode1Value = state;  // true = tocando agua
}

bool electrode1ReadApply()
{
	// ===== MODO INTERRUPTOR SIMPLE =====
	// Devuelve directamente el último estado leído (electrode1Value), sin media.
	bool state = electrode1Value;

	if (state)
		serial_println("RESULTADO -> ELECTRODE1 Tocando agua SÍ");
	else
		serial_println("RESULTADO -> ELECTRODE1 Tocando agua NO");

	return state;
}

void electrode2Read()
{
	// Pulso minimizado: se corta en cuanto el ADC captura la muestra (anti-electrólisis)
	unsigned int value = analogReadShortPulse(PIN_ELECTRODES_PULSE, PIN_ELECTRODE2_READ_DATA, ELECTRODE_2_READS_TIME_MICROSECONDS);

	#if PRINT_SERIAL_ELECTRODE_2_DEBUG_RAW
		serial_println("DEBUG E2 raw=" + String(value) + " (umbral=" + String(ELECTRODE_2_CONTACT_ANALOG_THRESHOLD) + ")");
	#endif

	// true = mojado. ELECTRODE_2_WATER_TRUE_CONTACT_WITH_HARDWARE_GET_VALUE elige la polaridad:
	//   1 = mojado se detecta con valor analógico BAJO  (value < umbral, lo habitual)
	//   0 = mojado se detecta con valor analógico ALTO  (value > umbral, hardware invertido)
	bool state = ELECTRODE_2_WATER_TRUE_CONTACT_WITH_HARDWARE_GET_VALUE
		? (value < ELECTRODE_2_CONTACT_ANALOG_THRESHOLD)
		: (value > ELECTRODE_2_CONTACT_ANALOG_THRESHOLD);

	// ===== MODO INTERRUPTOR SIMPLE =====
	// La última muestra manda directamente: sin conteo ni media.
	electrode2Value = state;  // true = tocando agua
}

bool electrode2ReadApply()
{
	// ===== MODO INTERRUPTOR SIMPLE =====
	// Devuelve directamente el último estado leído (electrode2Value), sin media.
	bool state = electrode2Value;

	if (state)
		serial_println("RESULTADO -> ELECTRODE2 Tocando agua SÍ");
	else
		serial_println("RESULTADO -> ELECTRODE2 Tocando agua NO");

	return state;
}

void beep(int pin, int frequency, int duration_ms)
{
	// Genera un tono en el pin especificado
	// pin: Pin donde se genera el tono
	// frequency: Frecuencia en Hz
	// duration_ms: Duración en milisegundos
	// Ejemplo: beep(PIN_BUZZER, 1000, 500);
	// Genera un tono de 1000 Hz durante 500 ms en el pin PIN_BUZZER
	if (frequency <= 0 || duration_ms <= 0) return;
	long period_us = 1000000L / frequency;
    long cycles = (long)frequency * duration_ms / 1000;

    for (long i = 0; i < cycles; i++) {
        digitalWrite(pin, HIGH);
        delayMicroseconds(period_us / 2);
        digitalWrite(pin, LOW);
        delayMicroseconds(period_us / 2);
    }
}

void leak_alarm_sound()
{
    // Alarma de fuga: patrón propio, más rápido y agudo que la alarma normal.
    // Sirena rápida de dos tonos alternos (tipo "wee-oo-wee-oo") para que
    // resulte más notoria que el "ni-no" lento de alarm_sound().
    for (int repeticion = 0; repeticion < 6; repeticion++)
    {
        beep(PIN_BUZZER, 4600, 60);  // tono agudo
        delay(25);
        beep(PIN_BUZZER, 2800, 60);  // tono grave relativo
        delay(25);
    }
    delay(400);  // Separación breve hasta la siguiente ráfaga
}

void alarm_sound()
{
    beep(PIN_BUZZER, 900*5, 100);   // Nota 2 (900 Hz, 300ms)
    delay(100);                   	// Pausa

    beep(PIN_BUZZER, 700*5, 100);   // Nota 2 (900 Hz, 300ms)
    delay(100);                   	// Pausa
}
