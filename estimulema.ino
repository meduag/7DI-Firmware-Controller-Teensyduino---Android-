#include <SPI.h>
// configuracion datos del dac
#define DAC_OP_MODE 1
#define DAC_SS_PIN 10
// #define PIN1 3
// #define PIN2 4
// definicion pinos del hardware
#define RELAY_CH_1 6 // rele del canal 1
#define RELAY_CH_2 7 // rele del canal 2
#define BuzzerPin 9 // rele del canal 2
#define STOP_BUTTON 23 // Boton de emergencia
#define SERIAL_TIMEOUT 50
/* Stimulation user configuration */
#define STIM_MAX 3410
#define STIM_ZERO 2047
#define STIM_MIN 684
#define INTERPHASE_DELAY 0 // us (offset de 30us)
#define INTERPULSE_DELAY 2000000 // us
#define INTERCHANNEL_DELAY 500000 // us
#define BUFFER_SIZE 10
/* Global variables      */
// For data on SPI communication
byte valueToWriteH = 0;
byte valueToWriteL = 0;
// Used in While-loops
boolean leave_loop;
int teste = 1;
struct therapy_t
{
	int ch_id;
	long freq; // Hz
	long therapy_Tfreq;
	int ampli; // milli amps
	int mA;
	long therapy_pw; // us
	long therapy_pw_r; // us
	long therapy_duration; // in seconds
};

struct micros_t
{
	long time1; // ms
	long time2; // ms
	long t_now; // ms
	long multi_us = 1000000;
	long multi_ms = 1000;
};

therapy_t t; // values for terapy
micros_t mt; // values for time counters and therapy time
micros_t pt; // values for time counters and period time
micros_t pwt; // values for time counters and pw time
/* First run */
void setup()
{
	/* System initialization */
	// Communication
	Serial.begin(2000000);
	Serial.setTimeout(SERIAL_TIMEOUT);
	// DAC
	pinMode(DAC_SS_PIN, OUTPUT);
	// Dac configurations - LOW/0: Enable SPI communication, HIGH/1: Disable communication
	digitalWrite(DAC_SS_PIN, 1);
	// Buzzer
	pinMode(BuzzerPin, OUTPUT);
	digitalWrite(BuzzerPin, 0);
	SPI.begin();
	// Relays
	pinMode(RELAY_CH_1, OUTPUT);
	pinMode(RELAY_CH_2, OUTPUT);
	// Relays configurations - LOW: Enable channel output, HIGH: Enable output for precharge resistor
	digitalWrite(RELAY_CH_1, 0); // LOW/0: precarga, HIGH/1: carga
	digitalWrite(RELAY_CH_2, 0); // LOW/0: precarga, HIGH/1: carga
	zeroChannels();
	leave_loop = false;
	// Predetermined values
	if (teste == 1)
	{
		t.freq = 300; // Hz
		t.ampli = 5; // milli amps
		t.therapy_duration = 1;
		t.therapy_pw = 500;
		t.ampli = 100;
		// configuration of initial values
		t.therapy_duration = t.therapy_duration * mt.multi_us; // in us
		t.therapy_Tfreq = mt.multi_us / t.freq; // period time in us
		t.therapy_pw_r = t.therapy_Tfreq - (t.therapy_pw * 2);
		// Este valor deberia ser pasado directamente por la GUI
		t.mA = map(t.ampli, 0, 100, 0, 2047);
	}
	Serial.println("Configurations ends");
	// delay(3000);
}

/* Eternal loop */
void loop()
{
	// Stay here until the app sends a start command
	// initializeSession(); // sets up channels and session types
	// BuzeerBIP(1,100);
	/*for (int i = 0; i < 60; i++)
	{
	t.ampli = i;
	t.mA = map(t.ampli, 0, 100, 0, 2047);
	printData();
	stimulationWave_Square();
	// }
	switch (i)
	{
	case 40:
	BuzeerBIP(1, 100);
	break;
	case 50:
	BuzeerBIP(2, 100);
	break;
	case 58:
	BuzeerBIP(3, 100);
	break;
	}
	}*/
	/* teste delay while  */
	// testeWhile();
	/* Calibration dac with max levels of current  */
	calibrationDAC(0, 100, 50);
}

/****************************************************************************************/
/************************************ System Functions **********************************/
/* Data reception */
void initializeSession()
{
	Serial.println("Waiting for conexion...");
	Serial.flush(); // clean buffer about data in
	char in_bytes[BUFFER_SIZE];
	while (Serial.available() > 0)
	{
		if (Serial.available() > 0)
		{
			Serial.readBytes(in_bytes, BUFFER_SIZE);
		}
		for (int i = 0; i < BUFFER_SIZE; i++)
		{
			Serial.print(in_bytes[i], HEX);
			Serial.print(" ");
		}
		Serial.println("\n\n\n");
		for (int i = 0; i < BUFFER_SIZE; i++)
		{
			Serial.print(in_bytes[i]);
			String val = "(" + String(i) + ")   ";
			Serial.print(val);
		}
		delay(10000);
		Serial.flush(); // clean buffer about data in
	}
}

/* Square wave geration */
void stimulationWave_Square()
{
	mt.t_now = micros();
	mt.time1 = micros();
	while (mt.time1 <= mt.t_now + t.therapy_duration)
	// therapy_duration
	{
		pt.t_now = micros();
		pt.time1 = micros();
		digitalWrite(RELAY_CH_1, 1); // LOW/0: precarga, HIGH/1: carga
		digitalWrite(RELAY_CH_2, 1); // LOW/0: precarga, HIGH/1: carga
		while (pt.time1 <= pt.t_now + t.therapy_Tfreq)
		// therapy_Tfreq
		{
			// ************************************** Positive pulse
			sendStimValue(0, 1,(STIM_ZERO + t.mA));
			sendStimValue(1, 1,(STIM_ZERO + t.mA));
			pwt.t_now = micros();
			pwt.time1 = micros();
			// aqui vou colocar la instrucion do DAC
			while (pwt.time1 <= pwt.t_now + t.therapy_pw)
			// therapy_pw
			{
				pwt.time1 = micros(); // therapy_pw
			}
			// ************************************** Negative pulse
			// aqui vou colocar la instrucion do DAC
			sendStimValue(0, 1,(STIM_ZERO - t.mA));
			sendStimValue(1, 1,(STIM_ZERO - t.mA));
			pwt.t_now = micros();
			pwt.time1 = micros();
			while (pwt.time1 <= pwt.t_now + t.therapy_pw)
			// therapy_pw
			{
				pwt.time1 = micros(); // therapy_pw
			}
			// ************************************* Rest pulse - 0 estimulation
			sendStimValue(0, 1, STIM_ZERO);
			sendStimValue(1, 1, STIM_ZERO);
			pwt.t_now = micros();
			pwt.time1 = micros();
			// aqui vou colocar la instrucion for other things like recived data
			while (pwt.time1 <= pwt.t_now + t.therapy_pw_r)
			// therapy_pw
			{
				pwt.time1 = micros(); // therapy_pw
			}
			pt.time1 = micros(); // therapy_Tfreq
		}
		mt.time1 = micros(); // therapy_duration
	}
	// Tempo final
	digitalWrite(RELAY_CH_1, 0); // LOW/0: precarga, HIGH/1: carga
	digitalWrite(RELAY_CH_2, 0); // LOW/0: precarga, HIGH/1: carga
	sendStimValue(0, 1,(STIM_ZERO + 20));
	/*
	mt.t_now = micros();
	mt.time1 = micros();
	while (mt.time1 <= mt.t_now + t.therapy_duration)
	// therapy_duration
	{
	mt.time1 = micros(); // therapy_duration
	}
	*/
}

void sendStimValue(int address, int operation_mode, uint16_t value)
{
	//Serial.print("Valor de entrada: ");
	//Serial.println(value);
	valueToWriteH = 0;
	valueToWriteL = 0;
	valueToWriteH = highByte(value);
	valueToWriteH = 0b00001111 & valueToWriteH;
	valueToWriteH = (address << 6) | (operation_mode << 4) | valueToWriteH;
	valueToWriteL = lowByte(value);
	// take the SS pin low to select the chip:
	digitalWrite(DAC_SS_PIN, LOW);
	// send in the address and value via SPI:
	SPI.transfer(valueToWriteH);
	SPI.transfer(valueToWriteL);
	// take the SS pin high to de-select the chip:
	digitalWrite(DAC_SS_PIN, HIGH);
}

void stimulationWave_Ramp()
{
	mt.t_now = micros();
	mt.time1 = micros();
	while (mt.time1 <= mt.t_now + t.therapy_duration)
	// therapy_duration
	{
		pt.t_now = micros();
		pt.time1 = micros();
		while (pt.time1 <= pt.t_now + t.therapy_Tfreq)
		// therapy_Tfreq
		{
			// ************************************** Positive pulse
			pwt.t_now = micros();
			pwt.time1 = micros();
			// aqui vou colocar la instrucion do DAC
			sendStimValue(0, 1, STIM_MAX);
			while (pwt.time1 <= pwt.t_now + t.therapy_pw)
			// therapy_pw
			{
				pwt.time1 = micros(); // therapy_pw
			}
			// ************************************** Negative pulse
			pwt.t_now = micros();
			pwt.time1 = micros();
			// aqui vou colocar la instrucion do DAC
			sendStimValue(0, 1, STIM_MIN);
			while (pwt.time1 <= pwt.t_now + t.therapy_pw)
			// therapy_pw
			{
				pwt.time1 = micros(); // therapy_pw
			}
			// ************************************* Rest pulse - 0 estimulation
			pwt.t_now = micros();
			pwt.time1 = micros();
			// aqui vou colocar la instrucion for other things like recived data
			sendStimValue(0, 1, STIM_ZERO);
			while (pwt.time1 <= pwt.t_now + t.therapy_pw_r)
			// therapy_pw
			{
				pwt.time1 = micros(); // therapy_pw
			}
			pt.time1 = micros(); // therapy_Tfreq
		}
		mt.time1 = micros(); // therapy_duration
	}
	// Tempo final
	mt.t_now = micros();
	mt.time1 = micros();
	sendStimValue(0, 1, STIM_ZERO);
	while (mt.time1 <= mt.t_now + t.therapy_duration)
	// therapy_duration
	{
		mt.time1 = micros(); // therapy_duration
	}
}

void testeWhile()
{
	mt.t_now = micros();
	mt.time1 = micros();
	while (mt.time1 <= mt.t_now + t.therapy_duration)
	{
		pt.t_now = micros();
		pt.time1 = micros();
		while (pt.time1 <= pt.t_now + t.therapy_Tfreq)
		{
			pwt.t_now = micros();
			pwt.time1 = micros();
			sendStimValue(0, 1, STIM_MAX);
			while (pwt.time1 <= pwt.t_now + t.therapy_pw)
			{
				pwt.time1 = micros(); // therapy_pw
			}
			pwt.t_now = micros();
			pwt.time1 = micros();
			sendStimValue(0, 1, STIM_MIN);
			while (pwt.time1 <= pwt.t_now + t.therapy_pw)
			{
				pwt.time1 = micros(); // therapy_pw
			}
			pwt.t_now = micros();
			pwt.time1 = micros();
			sendStimValue(0, 1, STIM_ZERO);
			while (pwt.time1 <= pwt.t_now + t.therapy_pw_r)
			{
				pwt.time1 = micros(); // therapy_pw
			}
			pt.time1 = micros(); // therapy_Tfreq
		}
		mt.time1 = micros(); // therapy_duration
	}
	mt.t_now = micros();
	mt.time1 = micros();
	while (mt.time1 <= mt.t_now + t.therapy_duration)
	{
		mt.time1 = micros(); // therapy_duration
	}
}

void calibrationDAC(int printValue, int duration, int dBip)
{
	if (printValue == 1)
	{
		printData();
		// BuzeerBIP(1, dBip);
		sendStimValue(0, 1, STIM_MAX);
		sendStimValue(1, 1, STIM_MAX);
		delay(duration);
		// BuzeerBIP(1, dBip);
		sendStimValue(0, 1, STIM_MIN);
		sendStimValue(1, 1, STIM_MIN);
		delay(duration);
		// BuzeerBIP(1, dBip);
		sendStimValue(0, 1, STIM_ZERO);
		sendStimValue(1, 1, STIM_ZERO);
		delay(duration);
	}
	if (printValue == 0)
	{
		digitalWrite(RELAY_CH_1, 1); // LOW/0: precarga, HIGH/1: carga
		digitalWrite(RELAY_CH_2, 1); // LOW/0: precarga, HIGH/1: carga
		for (int i = 0; i < 100; i++)
		{
			t.ampli = i;
			if(i == 50){
				BuzeerBIP(1, 100);
			}
			t.mA = map(t.ampli, 0, 100, 0, 1363);
			printData();
			// BuzeerBIP(1, 100);
			sendStimValue(0, 1, STIM_ZERO + t.mA);
			sendStimValue(1, 1, STIM_ZERO + t.mA);
			delay(duration);
			// BuzeerBIP(1, 100);
			t.mA = map(t.ampli, 0, 100, 0, 1300);
			sendStimValue(0, 1, STIM_ZERO - t.mA);
			sendStimValue(1, 1, STIM_ZERO - t.mA);
			delay(duration);
			// BuzeerBIP(1, 100);
			sendStimValue(0, 1, STIM_ZERO);
			sendStimValue(1, 1, STIM_ZERO);
			delay(duration);
		}
		digitalWrite(RELAY_CH_1, 0); // LOW/0: precarga, HIGH/1: carga
		digitalWrite(RELAY_CH_2, 0); // LOW/0: precarga, HIGH/1: carga
		BuzeerBIP(1, 100);
	}
}

void printData()
{
	Serial.print("Tiempo de terapia: ");
	Serial.println(t.therapy_duration);
	Serial.print("Frecuencia: ");
	Serial.println(t.freq);
	Serial.print("Periodo: ");
	Serial.println(t.therapy_Tfreq);
	Serial.print("Largura de Pulso: ");
	Serial.println(t.therapy_pw);
	Serial.print("tiempo faltante: ");
	Serial.println(t.therapy_pw_r);
	Serial.print("Corriente basico: ");
	Serial.println(t.ampli);
	Serial.print("Decimal DAC: ");
	Serial.println(t.mA);
	Serial.print("Decimal DAC positivo: ");
	Serial.println(STIM_ZERO + t.mA);
	Serial.print("Decimal DAC negativo: ");
	Serial.println(STIM_ZERO - t.mA);
	Serial.println(" ");
}

void zeroChannels()
{
	digitalWrite(RELAY_CH_1, 0);
	digitalWrite(RELAY_CH_2, 0);
	sendStimValue(0, 1,(uint16_t) (STIM_ZERO + 20));
	sendStimValue(1, 1,(uint16_t) (STIM_ZERO + 20));
}

void BuzeerBIP(int sel, int duration)
{
	switch (sel)
	{
		case 1:
			digitalWrite(BuzzerPin, 1);
			delay(duration);
			digitalWrite(BuzzerPin, 0);
			break;
		case 2:
			digitalWrite(BuzzerPin, 1);
			delay(duration);
			digitalWrite(BuzzerPin, 0);
			delay(duration);
			digitalWrite(BuzzerPin, 1);
			delay(duration);
			digitalWrite(BuzzerPin, 0);
			break;
		case 3:
			digitalWrite(BuzzerPin, 1);
			delay(duration);
			digitalWrite(BuzzerPin, 0);
			delay(duration);
			digitalWrite(BuzzerPin, 1);
			delay(duration);
			digitalWrite(BuzzerPin, 0);
			delay(duration);
			digitalWrite(BuzzerPin, 1);
			delay(duration);
			digitalWrite(BuzzerPin, 0);
			break;
	}
}
