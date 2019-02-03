#include <OneButton.h>
#include <DHT.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>

SoftwareSerial _serialOutput(10, 11); // RX, TX

#define SOIL_MOISTURE_PIN A0
#define LIGHT_BUTTON_PIN 5
#define LIGHT_RELAY_OUTPUT_PIN 7
#define PUMP_BUTTON_PIN 6
#define PUMP_RELAY_OUTPUT_PIN 8
#define DHT_PIN 4
#define DHTTYPE DHT21

#define SOIL_MOISTURE_MAX_VALUE 610
#define SOIL_MOISTURE_MIN_VALUE 310

#define INIT_SCREEN_DELAY_MS 1000
#define DISPLAY_UPDATE_EVERY 1000
#define READ_SOIL_MOISTURE_EVERY 100
#define WRITE_VALUES_TO_SERIAL_EVERY 1000
#define READ_DHT_EVERY 1000
#define INVERT_RELAY_SIGNAL true

DHT _dhtSensor(DHT_PIN, DHTTYPE);
LiquidCrystal_I2C _lcdDisplay(0x27, 20, 4);
OneButton _lightButton(LIGHT_BUTTON_PIN, true);
OneButton _pumpButton(PUMP_BUTTON_PIN, true);

bool _isLightEnabled = false;
bool _isPumpEnabled = false;

double _soilMoisture;
double _temperature;
double _humidity;

double _displaySoilMoisture;
double _displayHumidity;
double _displayTemperature;

unsigned long _lastUpdateDisplayTime;
unsigned long _lastReadSoilTime;
unsigned long _lastSerialWriteTime;
unsigned long _lastReadDhtSensor;

void setup() {
	Serial.begin(9600);

	pinMode(PUMP_RELAY_OUTPUT_PIN, OUTPUT);
	pinMode(LIGHT_RELAY_OUTPUT_PIN, OUTPUT);

	if (INVERT_RELAY_SIGNAL == true)
	{
		digitalWrite(PUMP_RELAY_OUTPUT_PIN, HIGH);
		digitalWrite(LIGHT_RELAY_OUTPUT_PIN, HIGH);
	}
	else
	{
		digitalWrite(PUMP_RELAY_OUTPUT_PIN, LOW);
		digitalWrite(LIGHT_RELAY_OUTPUT_PIN, LOW);
	}

	_lastUpdateDisplayTime = millis();
	_lastSerialWriteTime = millis();
	_lastReadSoilTime = millis();
	_lastReadDhtSensor = millis();

	_lightButton.attachClick(LightButtonClick);
	_pumpButton.attachClick(PumpButtonClick);

	_dhtSensor.begin();
	_serialOutput.begin(9600);

	InitializeLcd();
}

void loop() {

	if (millis() - _lastReadSoilTime > READ_SOIL_MOISTURE_EVERY)
	{
		ReadSoilMoisture();
		_lastReadSoilTime = millis();
	}

	if (millis() - _lastReadDhtSensor > READ_DHT_EVERY)
	{
		ReadDhtSensor();
		_lastReadDhtSensor = millis();
	}

	if (millis() - _lastUpdateDisplayTime > DISPLAY_UPDATE_EVERY)
	{
		SynchronizeSensorValuesAndDisplay();
		UpdateDisplayValues();
		_lastUpdateDisplayTime = millis();
	}
	
	if (millis() - _lastSerialWriteTime > WRITE_VALUES_TO_SERIAL_EVERY)
	{
		WriteValuesToSerial();
		_lastSerialWriteTime = millis();
	}
	_lightButton.tick();
	_pumpButton.tick();
 }

void LightButtonClick()
{
	_isLightEnabled = !_isLightEnabled;

	if (INVERT_RELAY_SIGNAL == true)
	{
		if (_isLightEnabled == true)
			digitalWrite(LIGHT_RELAY_OUTPUT_PIN, LOW);
		else
			digitalWrite(LIGHT_RELAY_OUTPUT_PIN, HIGH);
	}
	else
	{
		if (_isLightEnabled == true)
			digitalWrite(LIGHT_RELAY_OUTPUT_PIN, HIGH);
		else
			digitalWrite(LIGHT_RELAY_OUTPUT_PIN, LOW);
	}	

	UpdateButtonsInfoOnScreen();
}

void PumpButtonClick()
{
	_isPumpEnabled = !_isPumpEnabled;

	if (INVERT_RELAY_SIGNAL == true)
	{
		if (_isPumpEnabled == true)
			digitalWrite(PUMP_RELAY_OUTPUT_PIN, LOW);
		else
			digitalWrite(PUMP_RELAY_OUTPUT_PIN, HIGH);
	}
	else
	{
		if (_isPumpEnabled == true)
			digitalWrite(PUMP_RELAY_OUTPUT_PIN, HIGH);
		else
			digitalWrite(PUMP_RELAY_OUTPUT_PIN, LOW);
	}

	UpdateButtonsInfoOnScreen();
}

void InitializeLcd()
{
	_lcdDisplay.init();
	_lcdDisplay.backlight();
	_lcdDisplay.clear();

	_lcdDisplay.setCursor(4, 1);
	_lcdDisplay.print("Growbox 2000");
	_lcdDisplay.setCursor(8, 2);
	_lcdDisplay.print("v1.0");

	delay(INIT_SCREEN_DELAY_MS);

	_lcdDisplay.clear();
	_lcdDisplay.setCursor(0, 0);
	_lcdDisplay.print("Temperature: ");
	_lcdDisplay.setCursor(0, 1);
	_lcdDisplay.print("Humidity:");
	_lcdDisplay.setCursor(0, 2);
	_lcdDisplay.print("Soil moisture:");
	_lcdDisplay.setCursor(0, 3);
	_lcdDisplay.print("Light: ");
	_lcdDisplay.setCursor(11, 3);
	_lcdDisplay.print("Pump: ");
}

void ReadSoilMoisture()
{
	int sensorValue = analogRead(SOIL_MOISTURE_PIN);
	_soilMoisture = mapDouble(sensorValue, SOIL_MOISTURE_MAX_VALUE, SOIL_MOISTURE_MIN_VALUE, 0, 100);

	if (_soilMoisture < 0)
		_soilMoisture = 0;
	else if (_soilMoisture > 100)
		_soilMoisture = 100;
}

void ReadDhtSensor()
{
	float temperatureReading = _dhtSensor.readTemperature();
	float humidityReading = _dhtSensor.readHumidity();

	if (isnan(temperatureReading) == false)
		_temperature = temperatureReading;

	if (isnan(humidityReading) == false)
		_humidity = humidityReading;
}

void UpdateDisplayValues()
{
	_lcdDisplay.setCursor(12, 0);
	_lcdDisplay.print("        ");

	_lcdDisplay.setCursor(12, 0);
	_lcdDisplay.print(_displayTemperature, 1);
	_lcdDisplay.print((char)223);
	_lcdDisplay.print("C");

	_lcdDisplay.setCursor(9, 1);
	_lcdDisplay.print("           ");

	_lcdDisplay.setCursor(9, 1);
	_lcdDisplay.print(_displayHumidity, 1);
	_lcdDisplay.print("%");

	_lcdDisplay.setCursor(14, 2);
	_lcdDisplay.print("      ");

	_lcdDisplay.setCursor(14, 2);
	_lcdDisplay.print(_displaySoilMoisture, 1);
	_lcdDisplay.print("%");

	UpdateButtonsInfoOnScreen();
}

void UpdateButtonsInfoOnScreen()
{
	_lcdDisplay.setCursor(7, 3);
	_lcdDisplay.print("   ");

	_lcdDisplay.setCursor(7, 3);
	if (_isLightEnabled == true)
		_lcdDisplay.print("ON");
	else
		_lcdDisplay.print("OFF");

	_lcdDisplay.setCursor(17, 3);
	_lcdDisplay.print("   ");

	_lcdDisplay.setCursor(17, 3);
	if (_isPumpEnabled == true)
		_lcdDisplay.print("ON");
	else
		_lcdDisplay.print("OFF");
}

void SynchronizeSensorValuesAndDisplay()
{
	_displaySoilMoisture = _soilMoisture;
	_displayTemperature = _temperature;
	_displayHumidity = _humidity;
}

double mapDouble(double x, double in_min, double in_max, double out_min, double out_max)
{
	return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void WriteValuesToSerial()
{
	String outputJson = "{ \"Temperature\": " + (String)_temperature + ", \"Humidity\": " + (String)_humidity + ", \"SoilMoisture\": " + (String)_soilMoisture + ", \"IsPumpEnabled\": " + (String)_isPumpEnabled + ", \"IsLightEnabled\": " + (String)_isLightEnabled + " }";
	Serial.println(outputJson);
	_serialOutput.println(outputJson);
}