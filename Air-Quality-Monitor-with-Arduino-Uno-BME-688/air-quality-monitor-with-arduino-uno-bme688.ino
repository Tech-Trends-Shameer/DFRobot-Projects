#include "DFRobot_BME680_I2C.h"
#include "Wire.h"
#include <LiquidCrystal_I2C.h>

DFRobot_BME680_I2C bme(0x76);          // same address as the DFRobot example
LiquidCrystal_I2C lcd(0x27, 16, 2);

const int BASELINE_SAMPLES = 10;
float baselineSum = 0;
int samples = 0;
float baseline = 0;
int screen = 0;

void setup() {
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("DFRobot");
  lcd.setCursor(0, 1);
  lcd.print("BME688 Monitor");
  delay(2000);

  // Same init loop as DFRobot's example, with LCD feedback
  uint8_t rslt = 1;
  while (rslt != 0) {
    rslt = bme.begin();
    if (rslt != 0) {
      Serial.println(F("bme begin failure"));
      lcd.clear();
      lcd.print("Sensor not found");
      delay(2000);
    }
  }
  Serial.println(F("bme begin successful"));
  lcd.clear();
  lcd.print("Sensor ready");
  delay(1000);
}

void loop() {
  bme.startConvert();
  delay(1000);
  bme.update();

  float temp = bme.readTemperature() / 100.0;       // C
  float hum  = bme.readHumidity() / 1000.0;         // %
  float pres = bme.readPressure() / 100.0;          // Pa -> hPa
  float gas  = bme.readGasResistance() / 1000.0;    // ohm -> kOhm

  // Baseline from the first readings (start in clean air)
  if (samples < BASELINE_SAMPLES) {
    baselineSum += gas;
    samples++;
    if (samples == BASELINE_SAMPLES) baseline = baselineSum / samples;
  }

  const char* status;
  if (baseline == 0)             status = "Calibrating";
  else if (gas / baseline > 0.8) status = "Fresh";
  else if (gas / baseline > 0.5) status = "Moderate";
  else                           status = "Poor";

  // ---- Serial Monitor ----
  Serial.print(F("Temperature : ")); Serial.print(temp, 1); Serial.println(F(" C"));
  Serial.print(F("Humidity    : ")); Serial.print(hum, 1);  Serial.println(F(" %"));
  Serial.print(F("Pressure    : ")); Serial.print(pres, 1); Serial.println(F(" hPa"));
  Serial.print(F("Gas         : ")); Serial.print(gas, 1);  Serial.println(F(" kOhm"));
  Serial.print(F("Air Quality : ")); Serial.println(status);
  Serial.println(F("------------------------------"));

  // ---- LCD (rotating screens) ----
  lcd.clear();
  if (screen == 0) {
    lcd.setCursor(0, 0);
    lcd.print("Temp: "); lcd.print(temp, 1); lcd.print((char)223); lcd.print("C");
    lcd.setCursor(0, 1);
    lcd.print("Hum : "); lcd.print(hum, 1); lcd.print(" %");
  } else if (screen == 1) {
    lcd.setCursor(0, 0);
    lcd.print("Pres: "); lcd.print(pres, 1); lcd.print("hPa");
    lcd.setCursor(0, 1);
    lcd.print("Gas : "); lcd.print(gas, 1); lcd.print("kOhm");
  } else {
    lcd.setCursor(0, 0);
    lcd.print("Air quality:");
    lcd.setCursor(0, 1);
    lcd.print(status);
  }

  screen = (screen + 1) % 3;
  delay(1000);
}
