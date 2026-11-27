/*
  Tech Trends Shameer
  FireBeetle 2 ESP32-P4 + Fermion BME688 + 16x2 I2C LCD
  Two-Way WhatsApp Chat + Automatic Push Alerts via ThingESP + Twilio
*/

#include <WiFi.h>
#include <ThingESP.h>
#include <Wire.h>
#include "DFRobot_BME680_I2C.h"
#include <LiquidCrystal_I2C.h>

// ---------------- WiFi Credentials ----------------
char ssid[] = "WiFi Username";
char pass[] = "WiFi Password";

// ---------------- ThingESP Credentials ----------------
ThingESP32 thing("techtrends", "IoTProject", "techtrends");

// ---------------- Your WhatsApp Number ----------------
// Replace XXXXXXXXXX with your 10-digit mobile number (keep +91)
String yourWhatsAppNumber = "+91XXXXXXXXXX"; 

// ---------------- BME688 Sensor ----------------
DFRobot_BME680_I2C bme(0x76);

// ---------------- 16x2 I2C LCD ----------------
LiquidCrystal_I2C lcd(0x27, 16, 2);

byte degreeSymbol[8] = {
  0b01100,
  0b10010,
  0b10010,
  0b01100,
  0b00000,
  0b00000,
  0b00000,
  0b00000
};

// ---------------- Alert Thresholds ----------------
const float MAX_TEMP_C = 28.0;      // Set to 28.0°C for immediate testing (raise to 35.0 after test)
const float MAX_HUMIDITY = 85.0;    // Alert if humidity exceeds 85%
const float MIN_GAS_KOHM = 20.0;    // Alert if gas drops below 20 kOhms

// ---------------- Timing ----------------
const unsigned long READ_INTERVAL_MS = 2000;
const unsigned long ALERT_COOLDOWN_MS = 15 * 60 * 1000; // 15-minute cooldown between alerts
unsigned long lastReadTime = 0;
unsigned long lastAlertTime = 0;

// ---------------- Sensor Variables ----------------
float temperature = 0.0;
float humidity = 0.0;
float pressure = 0.0;
float gasKOhm = 0.0;
bool sensorAvailable = false;

// ==================================================
// READ BME688
// ==================================================
void readSensor() {
  bme.setGasHeater(320, 100);
  delay(30);
  bme.startConvert();
  delay(1000);
  bme.update();

  temperature = bme.readTemperature() / 100.0; // °C
  humidity = bme.readHumidity() / 1000.0;      // %
  pressure = bme.readPressure() / 100.0;       // hPa
  gasKOhm = bme.readGasResistance() / 1000.0;  // kOhms
}

// ==================================================
// UPDATE 16x2 LCD
// ==================================================
void updateLCD() {
  // Line 1: Temperature & Humidity
  lcd.setCursor(0, 0);
  lcd.print("T:");
  lcd.print(temperature, 1);
  lcd.write((byte)0);
  lcd.print("C  H:");
  lcd.print(humidity, 0);
  lcd.print("%   ");

  // Line 2: Pressure & Gas in kOhms
  lcd.setCursor(0, 1);
  lcd.print("P:");
  lcd.print(pressure, 0);
  lcd.print("  G:");
  lcd.print(gasKOhm, 1);
  lcd.print("k   ");
}

// ==================================================
// WHATSAPP COMMAND HANDLER (Two-Way Chat)
// ==================================================
String HandleResponse(String query) {
  query.trim();
  query.toLowerCase();

  Serial.print("WhatsApp Query: ");
  Serial.println(query);

  if (query == "temperature" || query == "temp") {
    return "🌡️ Temperature: " + String(temperature, 1) + " °C";
  }
  else if (query == "humidity") {
    return "💧 Humidity: " + String(humidity, 1) + " %";
  }
  else if (query == "pressure") {
    return "⏱️ Pressure: " + String(pressure, 1) + " hPa";
  }
  else if (query == "air" || query == "gas") {
    String airStatus = (gasKOhm < MIN_GAS_KOHM) ? "Poor (High VOCs)" : "Good";
    return "🍃 Air Quality: " + airStatus + "\nGas Resistance: " + String(gasKOhm, 2) + " kOhms";
  }
  else if (query == "status") {
    String response = "📊 Room Air Monitor\n\n";
    response += "Temp: " + String(temperature, 1) + " °C\n";
    response += "Humidity: " + String(humidity, 1) + " %\n";
    response += "Pressure: " + String(pressure, 1) + " hPa\n";
    response += "Gas: " + String(gasKOhm, 2) + " kOhms";
    return response;
  }
  else if (query == "help") {
    return "Available Commands:\ntemperature\nhumidity\npressure\nair\nstatus\nhelp";
  }
  else {
    return "Invalid command. Type 'help' for commands.";
  }
}

// ==================================================
// SETUP
// ==================================================
void setup() {
  Serial.begin(115200);
  delay(800);

  Wire.begin();

  // 1. Initialize LCD & Show Welcome Screens
  lcd.init();
  lcd.backlight();
  lcd.createChar(0, degreeSymbol);

  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print("Tech Trends");
  lcd.setCursor(4, 1);
  lcd.print("Shameer");
  delay(2000);

  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print("DFRobot Air");
  lcd.setCursor(0, 1);
  lcd.print("Quality Monitor");
  delay(2000);

  // 2. Initialize BME688
  if (bme.begin() != 0) {
    Serial.println("BME688 not found! Check wiring.");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("BME688 ERROR!");
    lcd.setCursor(0, 1);
    lcd.print("Check I2C Wiring");
    sensorAvailable = false;
  } else {
    Serial.println("BME688 detected!");
    sensorAvailable = true;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Connecting WiFi.");
  }

  // 3. Initialize ThingESP & WiFi
  // 3. Initialize ThingESP & WiFi
  thing.SetWiFi(ssid, pass);
  thing.initDevice();
  Serial.println("ThingESP initialized. Syncing settings...");

  // Wait 3 seconds so "[INFO] Device calls: enabled" arrives BEFORE loop() sends an alert
  for (int i = 0; i < 30; i++) {
    thing.Handle();
    delay(100);
  }
  lcd.clear();

}

// ==================================================
// LOOP
// ==================================================
void loop() {
  // 1. Listen for incoming WhatsApp commands
  thing.Handle();

  // 2. Read sensor, update LCD & check automatic alerts
  if (millis() - lastReadTime >= READ_INTERVAL_MS) {
    lastReadTime = millis();

    if (sensorAvailable) {
      readSensor();
      updateLCD();

      Serial.printf("Temp: %.1f C | Hum: %.1f %% | Press: %.1f hPa | Gas: %.2f kOhms\n", 
                    temperature, humidity, pressure, gasKOhm);

      bool alertTriggered = false;
      String alertMsg = "Room Air Alert:\n";

      if (temperature > MAX_TEMP_C) {
        alertMsg += "- High Temp: " + String(temperature, 1) + " °C\n";
        alertTriggered = true;
      }
      if (humidity > MAX_HUMIDITY) {
        alertMsg += "- High Humidity: " + String(humidity, 1) + " %\n";
        alertTriggered = true;
      }
      if (gasKOhm < MIN_GAS_KOHM && gasKOhm > 0) {
        alertMsg += "- Poor Air Quality: " + String(gasKOhm, 2) + " kOhms\n";
        alertTriggered = true;
      }

      if (alertTriggered && (millis() - lastAlertTime > ALERT_COOLDOWN_MS || lastAlertTime == 0)) {
        Serial.println(">>> SENDING ALERT VIA THINGESP <<<");
        thing.sendMsg(yourWhatsAppNumber, alertMsg);
        lastAlertTime = millis();
      }
    }
  }
}
