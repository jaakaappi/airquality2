#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <math.h>

// display deps
#include <hd44780.h>                       // main hd44780 header
#include <hd44780ioClass/hd44780_I2Cexp.h> // i2c expander i/o class header
#include "glyphs.h"
hd44780_I2Cexp lcd(0x27);
void printInfoToLcd();

// co2 deps
#include "MHZ19.h"
#include <SoftwareSerial.h>
#define MH_Z19_RX 19
#define MH_Z19_TX 18
MHZ19 co2Sensor;
SoftwareSerial co2Serial(MH_Z19_RX, MH_Z19_TX);
int co2 = -1;
int co2Stats[20] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

#include "Adafruit_SHTC3.h"
Adafruit_SHTC3 shtc3 = Adafruit_SHTC3();
float temperatureC = 0.0;
float relativeHumidity = 0.0;

#define MEASUREMENT_INTERVAL 60 * 1000
void getMeasurements();
int lastMeasurement = 0;

void setup()
{
  Serial.begin(115200);

  lcd.begin(20, 4);
  delay(100);

  lcd.createChar(0, glyph_one);
  lcd.createChar(1, glyph_two);
  lcd.createChar(2, glyph_three);
  lcd.createChar(3, glyph_four);
  lcd.createChar(4, glyph_five);
  lcd.createChar(5, glyph_six);
  lcd.createChar(6, glyph_seven);
  lcd.createChar(7, glyph_eight);

  co2Serial.begin(9600);
  co2Sensor.begin(co2Serial);
  // co2Sensor.autoCalibration(false);
  delay(100);

  if (!shtc3.begin())
  {
    Serial.println("Couldn't find SHTC3");
  }
  delay(100);

  getMeasurements();
  printInfoToLcd();

  Serial.println("Ready");
}

void loop()
{
  if (millis() - lastMeasurement > MEASUREMENT_INTERVAL)
  {
    getMeasurements();
    printInfoToLcd();
    lastMeasurement = millis();
  }
}

void getMeasurements()
{
  co2 = co2Sensor.getCO2();
  // move all values left 1 space
  for (int i = 0; i < 19; i++)
  {
    co2Stats[i] = co2Stats[i + 1];
  }
  co2Stats[19] = co2;

  sensors_event_t humidity, temperature;
  shtc3.getEvent(&humidity, &temperature);
  relativeHumidity = humidity.relative_humidity;
}

void statsToGraph(int stats[20])
{
  float min = 0;
  float max = 0;
  float step = 0;

  // Debug code
  // Serial.print("stats ");
  // for (int i = 0; i < 20; i++)
  // {
  //   Serial.print(String(stats[i]));
  //   if (i != 19)
  //     Serial.print(",");
  //   else
  //     Serial.println();
  // }

  for (int i = 0; i < 20; i++)
  {
    if (stats[i] < min)
      min = stats[i];
  }
  for (int i = 0; i < 20; i++)
  {
    if (stats[i] > max)
      max = stats[i];
  }

  Serial.println("min " + String(min));
  Serial.println("max " + String(max));

  lcd.setCursor(0, 0);
  for (int i = 0; i <= 19; i++)
  {
    int value = int((stats[i] - max / 2) * 2 / max * 8.0f);
    // Debug code
    // Serial.print(String(stats[i]) + " ");
    // Serial.print(String((stats[i] - max / 2) * 2 / max * 8.0f) + " ");
    // Serial.print(String(value) + " ");
    // Serial.println();
    if (value <= 0)
      lcd.write((uint8_t)20);
    else if (value >= 8)
      lcd.write((uint8_t)7);
    else
      lcd.write((uint8_t)value - 1);
  }

  lcd.setCursor(0, 1);
  for (int i = 0; i <= 19; i++)
  {
    int value = int(stats[i] / max * 16.0f);
    // Debug code
    // Serial.print(String(stats[i]) + " ");
    // Serial.print(String(stats[i] / max * 16.0f) + " ");
    // Serial.print(String(value) + " ");
    // Serial.println();
    if (value <= 0)
      lcd.write((uint8_t)20);
    else if (value >= 8)
      lcd.write((uint8_t)7);
    else
      lcd.write((uint8_t)value - 1);
  }
}

void printInfoToLcd()
{
  lcd.clear();
  lcd.setCursor(0, 0);

  statsToGraph(co2Stats);

  lcd.setCursor(0, 2);
  lcd.print("CO2 " + String(co2) + " ppm");
  lcd.setCursor(0, 3);
  lcd.print("T " + String(temperatureC) + "\xDF");
  lcd.print("C rH " + String(relativeHumidity) + "%");
}