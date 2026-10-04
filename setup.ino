void setup()
{
	// El relé se apaga lo antes posible: durante el reset del ATmega el pin queda en
	// alta impedancia (entrada) y, como el relé se activa por nivel BAJO (RELAY_ON=0),
	// un pin flotante puede energizarlo unos instantes. Dejarlo OFF ya aquí evita el
	// pulso de ~1 s al reiniciar.
	pinMode(PIN_RELE, OUTPUT);
	digitalWrite(PIN_RELE, RELAY_OFF);

	if (PRINT_SERIAL) Serial.begin(115200);

	setup_jumpers();  // Inicia los jumpers (forzar relé ON/OFF)

	pinMode(PIN_SWITCH_LEDS_HIGH, INPUT);  // Inicia el jumper de leds alto
	pinMode(PIN_GND_PWM_LED, OUTPUT);  // Inicia el canal PWM de brillo
	analogWrite(PIN_GND_PWM_LED, LED_ANALOG_VALUE_BRIGHT_LOW);
	updateLeds();

	pinMode(PIN_SCAN_LED, OUTPUT);

	if (PROCESS_LEVEL) 
	{
		// Inicia el nivel si la constante PROCESS_LEVEL es 1
		setup_level();
	}

	// Inicia cada electrodo solo si su constante PROCESS_ELECTRODE_x está a 1
	if (PROCESS_ELECTRODE_1) setup_electrode1();
	if (PROCESS_ELECTRODE_2) setup_electrode2();

	setup_relay();

	ledsTest();  // Prueba de leds
	ledsAllOff();  // Apaga todos los leds
	updateBrightLeds();  // Actualiza el brillo de los LEDs

	setup_buzzer(); // Inicia el buzzer
}

void setup_jumpers()
{
	// Inicia los Jumpers
	pinMode(PIN_SWITCH_FORCE_RELAY_ON, INPUT);
	pinMode(PIN_SWITCH_FORCE_RELAY_OFF, INPUT);
	lastSwitchForceRelayOn = digitalRead(PIN_SWITCH_FORCE_RELAY_ON);
	lastSwitchForceRelayOff = digitalRead(PIN_SWITCH_FORCE_RELAY_OFF);
}

void setup_buzzer()
{
	// Inicia el buzzer
	pinMode(PIN_BUZZER, OUTPUT);
	digitalWrite(PIN_BUZZER, HIGH);
	delay(200);
	digitalWrite(PIN_BUZZER, LOW);
}

void setup_level()
{
	// Inicia la alimentación de pulso del nivel
	pinMode(PIN_LEVEL_PULSE, OUTPUT);

	// Inicia el lector del nivel
	pinMode(PIN_LEVEL_READ_DATA, INPUT);

	// Inicia el led del nivel
	pinMode(PIN_LEVEL_LED, OUTPUT);
}

void setup_electrode1()
{
	// Inicia la alimentación de pulso del electrodo 1
	pinMode(PIN_ELECTRODES_PULSE, OUTPUT);

	// Inicia el lector del electrodo1
	pinMode(PIN_ELECTRODE1_READ_DATA, INPUT);

	// Inicia el led del electrodo 1
	pinMode(PIN_ELECTRODE1_LED, OUTPUT); 
}

void setup_electrode2()
{
	// Inicia la alimentación de pulso del electrodo 2
	pinMode(PIN_ELECTRODES_PULSE, OUTPUT);

	// Inicia el lector del electrodo2
	pinMode(PIN_ELECTRODE2_READ_DATA, INPUT);

	// Inicia el led del electrodo 2
	pinMode(PIN_ELECTRODE2_LED, OUTPUT); 
}

void setup_relay()
{
	// Inicia el led del relé
	pinMode(PIN_RELAY_LED, OUTPUT); 
}
