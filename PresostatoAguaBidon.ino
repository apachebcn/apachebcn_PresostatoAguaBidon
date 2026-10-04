/*
* ==============================================================
*    SENSOR DE NIVEL O PRESOSTATO DE LAVADORA O EQUIVALENTES
*               Y SENSOR DE NIVEL CON ELECTRODO
* ==============================================================
*
* Un presostato es un interruptor que se activa o desactiva en función de la presión de un fluido, como el aire o el agua.
* 
* El electrodo consiste en 2 cables que se usará como refuerzo al nivel electrónico, situandose un poco más arriba de este.
* Cuando el agua llega a los cables, es detectado por el arduino, y hace el trabajo que debió hacer el prestato.
* 
* Este circuito y código se ha fabricado para el proyecto de misolarcasero.com
* 
* ==============================================================
*/


/*
*    +---------------------------------------------+
*    |                                             |
*    |           DISPOSITIVO SWITCH LEVEL          |
*    |                                             |
*    +---------------------------------------------+
*        |    PIN 1    |         |    PIN 2    |
*        |    PULSO    |         |    DATA     |
*        +-------------+         +-------------+
*               |                       |                  
*               |                       |                  
*   (PIN_LEVEL_PULSE #3) (PIN_LEVEL_READ_DATA #2)
*                                       |                  
*                                       +------| 4,7KOhms|------+ (GND)
*/

/*
*    +---------------------------------------------------------------------+
*    |                                                                     |
*    |                  TRANSISTOR BC337 PARA EL ELECTRODO                 |
*    |                                                                     |
*    +---------------------------------------------------------------------+
*        |    PATA 1   |         |   PATA 2    |         |   PATA 3    |
*        |   EMISOR    |         |    BASE     |         |   COLECTOR  |--------------- PIN_ELECTRODE1_READ_DATA
*        +-------------+         +-------------+         +-------------+
*               |                       |                       |
*               |                       |                       |
*              GND                   ELECTRODO                 10K       
*                                  4K7 OR 10K PULLDOWN         VCC


/*
* ELECTRODOS:
*    - CABLE 1: PATA 2 del transistor
*    - CABLE 2: PIN_ELECTRODES_PULSE
*/

#include "config_general.h"
#include "config_pins.h"

unsigned long lastProcessLoopTime = 0;  // Tiempo del último loop

unsigned long lastProcessAlarmTime = 0;  // Tiempo del último pitido de alarma

unsigned long lastProcessLeakAlarmTime = 0;  // Tiempo del último pitido de alarma de fuga

bool lastSwitchForceRelayOn = 0;  // Último estado del jumper de forzar relé ON
bool changedSwitchForceRelayOn = 0;  // Indica si ha cambiado el estado del jumper de forzar relé ON

bool lastSwitchForceRelayOff = 0;  // Último estado del jumper de forzar relé OFF
bool changedSwitchForceRelayOff = 0;  // Indica si ha cambiado el estado del jumper de forzar relé OFF

bool lastSwitchLedsHigh = 0;  // Último estado del jumper de leds brillo alto
bool changedSwitchLedsHigh = 0;  // Indica si ha cambiado el estado del jumper de leds brillo alto

bool alarm = 0;  // Estado de la alarma
bool levelValue = !LEVEL_WATER_TRUE_CONTACT_WITH_HARDWARE_GET_VALUE;  // Estado del nivel
bool levelRawValue = !LEVEL_WATER_TRUE_CONTACT_WITH_HARDWARE_GET_VALUE;  // Última lectura EN BRUTO del nivel (1 = pin 2 en LOW)
bool electrode1Value = false;  // Estado del electrodo 1 (true = tocando agua)
bool electrode2Value = false;  // Estado del electrodo 2 (true = tocando agua)

unsigned long levelSamplesOn = 0;  // Muestras "tocando agua" del nivel (para la media)
unsigned long levelSamplesOff = 0;  // Muestras "seco" del nivel (para la media)

unsigned long delay_loop_ms = 0;  // Tiempo que ha tardado en ejecutarse el loop
unsigned long time_loop_start = 0;  // Tiempo de inicio del loop

bool valueSwitchLedsHigh = false;
bool valueSwitchForceRelayOn = false;
bool valueSwitchForceRelayOff = false;

bool relay_on = 0;  // Estado del relé

bool leakAlarm = 0;  // Alarma de fuga (bloquea el sistema hasta reiniciar el Arduino)
unsigned long relayOnSince = 0;  // Tiempo en que el relé se puso ON (para alarma de fuga)

bool intermitentEnable = 0;  // Habilita el parpadeo de los leds
bool intermitentStepOn = false;  // Estado del parpadeo
unsigned long intermitentLastTime = 0;  // Tiempo del último cambio de estado del parpadeo
const unsigned long INTERMITENT_INTERVAL = 250;  // Intervalo de tiempo del par


#include "functions.h"

void loop()
{
	delay(10);

	if (millis() - intermitentLastTime >= INTERMITENT_INTERVAL) {
		intermitentStepOn = !intermitentStepOn;
		intermitentLastTime = millis();
		updateLeds();
	}

	if (PROCESS_JUMPERS)
	{
		valueSwitchLedsHigh = digitalRead(PIN_SWITCH_LEDS_HIGH);
		valueSwitchForceRelayOn = digitalRead(PIN_SWITCH_FORCE_RELAY_ON);
		valueSwitchForceRelayOff = digitalRead(PIN_SWITCH_FORCE_RELAY_OFF);
	}
	else
	{
		valueSwitchLedsHigh = false;
		valueSwitchForceRelayOn = false;
		valueSwitchForceRelayOff = false;
	}

	if (leakAlarm || valueSwitchForceRelayOn || valueSwitchForceRelayOff) intermitentEnable = 1; else intermitentEnable = 0;

	if (alarm)  // Si la alarma está activada
	{
		if (TIME_ALARM)  // Si ha pasado el tiempo de alarma
		{
			alarm_sound();  // Suena la alarma
			lastProcessAlarmTime = millis();
		}
	}

	if (leakAlarm)  // Si la alarma de fuga está activada
	{
		if (TIME_LEAK_ALARM)  // Si ha pasado el tiempo de la alarma de fuga
		{
			leak_alarm_sound();  // Suena la sirena rápida de la alarma de fuga
			lastProcessLeakAlarmTime = millis();
		}
	}

	if (LEAK_ALARM_ENABLE) checkLeakAlarm();  // Comprueba si hay riesgo de fuga

	// Detección de cambios en los jumpers (solo si el perfil los procesa)
	if (PROCESS_JUMPERS)
	{
		changedSwitchForceRelayOn = isSwitchChanged(PIN_SWITCH_FORCE_RELAY_ON, lastSwitchForceRelayOn);
		changedSwitchForceRelayOff = isSwitchChanged(PIN_SWITCH_FORCE_RELAY_OFF, lastSwitchForceRelayOff);
		changedSwitchLedsHigh = isSwitchChanged(PIN_SWITCH_LEDS_HIGH, lastSwitchLedsHigh);

		if (changedSwitchLedsHigh)  // Si ha cambiado el estado del jumper de leds brillo alto
		{
			lastSwitchLedsHigh = valueSwitchLedsHigh;  // Actualiza el último estado del jumper de leds brillo alto
			updateBrightLeds();
		}

		if (changedSwitchForceRelayOn)  // Si ha cambiado el estado del jumper de forzar relé ON
		{
			relay_on = getRelayActionOn(valueSwitchForceRelayOn, valueSwitchForceRelayOff, levelValue, electrode1Value, electrode2Value);
			refreshRelay(relay_on);  // Actualiza el estado del relé
		}

		if (changedSwitchForceRelayOff)  // Si ha cambiado el estado del jumper de forzar relé OFF
		{
			relay_on = getRelayActionOn(valueSwitchForceRelayOn, valueSwitchForceRelayOff, levelValue, electrode1Value, electrode2Value);
			refreshRelay(relay_on);  // Actualiza el estado del relé
		}
	}

	// lecturas sensores
	// El nivel se muestrea cada iteración y su media se decide en TIME_LOOP.
	// Los electrodos NO se leen aquí: al ser interruptor simple, se leen una vez
	// por cada decisión, dentro del bloque TIME_LOOP.
	if (PROCESS_LEVEL) levelRead();

	// Si ha pasado el tiempo del loop o ha cambiado algún jumper
	if (TIME_LOOP)
    {
		time_loop_start = millis();

		serial_println("");
		serial_println("LOOP");

		ledOn(PIN_SCAN_LED);
		updateLeds();

		if (valueSwitchForceRelayOn) { serial_println(); serial_println("SWITCH CLOSED: PIN_SWITCH_FORCE_RELAY_ON");}
		if (valueSwitchForceRelayOff) { serial_println(); serial_println("SWITCH CLOSED: PIN_SWITCH_FORCE_RELAY_OFF");}

		if (LEAK_ALARM_ENABLE && leakAlarm) serial_println("ALARMA RIESGO DE FUGA ACTIVA: SISTEMA BLOQUEADO");

		// `alarm` NO se resetea aquí: RULES_FOR_ALARM usa V = estado anterior de la alarma.

		if (PROCESS_LEVEL)
		{
			levelValue = levelReadApply();
			if ((levelValue == LEVEL_WATER_TRUE_CONTACT_WITH_HARDWARE_GET_VALUE) == LEVEL_LED_ON_WHEN_WATER_CONTACT)
			{
				ledOn(PIN_LEVEL_LED);  // Testigo ON del Nivel en el led
			}
			else 
			{
				ledOff(PIN_LEVEL_LED);  // Testigo OFF del Nivel en el led
			}
		}

		if (PROCESS_ELECTRODE_1)
		{
			electrode1Read();  // Lectura fresca en cada decisión (interruptor simple)
			electrode1Value = electrode1ReadApply();
			if (electrode1Value == ELECTRODE_1_LED_ON_WHEN_WATER_CONTACT)
			{
				ledOn(PIN_ELECTRODE1_LED);  // Testigo ON del Electrodo1 en el led
			}
			else
			{
				ledOff(PIN_ELECTRODE1_LED);  // Testigo OFF del Electrodo1 en el led
			}
		}

		if (PROCESS_ELECTRODE_2)
		{
			electrode2Read();  // Lectura fresca en cada decisión (interruptor simple)
			electrode2Value = electrode2ReadApply();
			if (electrode2Value == ELECTRODE_2_LED_ON_WHEN_WATER_CONTACT)
			{
				ledOn(PIN_ELECTRODE2_LED);  // Testigo ON del Electrodo2 en el led
			}
			else
			{
				ledOff(PIN_ELECTRODE2_LED);  // Testigo OFF del Electrodo2 en el led
			}
		}

		alarm = alarmRulesApply();  // Decisión según las reglas RULES_FOR_ALARM (led + buzzer)

		relay_on = getRelayActionOn(valueSwitchForceRelayOn, valueSwitchForceRelayOff, levelValue, electrode1Value, electrode2Value);
		refreshRelay(relay_on);  // Actualiza el estado del relé

		serial_println();
		if (relay_on)
		{
			serial_println("RELE ON (1)");

			// Si la alarma de fuga está activa y el relé se activó en automático (no con el
			// jumper de llenado manual), avisa del tiempo que queda para que salte el bloqueo.
			if (LEAK_ALARM_ENABLE && !(PROCESS_JUMPERS && valueSwitchForceRelayOn))
			{
				unsigned long remainingSeconds = LEAK_ALARM_TIME_SECONDS;
				if (relayOnSince != 0)
				{
					unsigned long elapsedSeconds = (millis() - relayOnSince) / 1000UL;
					remainingSeconds = (elapsedSeconds >= LEAK_ALARM_TIME_SECONDS)
						? 0UL : (LEAK_ALARM_TIME_SECONDS - elapsedSeconds);
				}
				String remainingText = "TIEMPO PARA ALARMA DE FUGA: " + String(remainingSeconds) + " s";
				if (remainingSeconds > 60)
				{
					remainingText += " (" + String(remainingSeconds / 60) + " min)";
				}
				serial_println(remainingText);
			}
		}
		else
		{
			serial_println("RELE OFF (0)");
		}

		lastSwitchForceRelayOn = valueSwitchForceRelayOn;  // Actualiza el último estado del jumper de forzar relé ON
		lastSwitchForceRelayOff = valueSwitchForceRelayOff;  // Actualiza el último estado del jumper de forzar relé OFF

		delay_loop_ms = millis() - time_loop_start;  // Tiempo que ha tardado en ejecutarse el loop
		if (delay_loop_ms < 50) delay(50 - delay_loop_ms);  // Pequeña pausa para la visualización del led PIN_SCAN_LED
		
		ledOff(PIN_SCAN_LED);
		updateLeds();

		lastProcessLoopTime = millis(); // Actualiza el tiempo del último loop
	}

	applyLeakAlarm();  // Aplica la alarma de fuga (relé OFF + luz de alarma) según la variable leakAlarm
}
