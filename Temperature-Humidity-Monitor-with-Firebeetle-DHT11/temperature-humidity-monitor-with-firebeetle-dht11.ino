#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "DHT.h"

// Changed to Pin 5 to prevent the ESP32-P4 boot-crash/panic
#define DHTPIN 5     
#define DHTTYPE DHT11

// Match your working test configuration exactly (No custom Wire pins defined globally)
LiquidCrystal_I2C lcd(0x27, 16, 2); 

// Create a pointer for the sensor instead of a global object to protect the boot loader
DHT *dht = nullptr; 

unsigned long lastCheck = 0;

void setup() {
  Serial.begin(115200);
  delay(500);
  
  Serial.println("1. Initializing LCD strictly matching working test parameters...");
  lcd.init();
  lcd.backlight();
  lcd.clear();
  
  // THIS WILL PRINT INSTANTLY ON BOOT
  lcd.setCursor(0, 0);
  lcd.print("DFRobot");
  lcd.setCursor(0, 1);
  lcd.print("ESP32 P4 Station ");
  
  Serial.println("2. Display initialized successfully. Safeguarding sensor instantiation...");
  delay(2000); 
  
  // Safe runtime allocation of the DHT object after the display is fully active
  dht = new DHT(DHTPIN, DHTTYPE);
  dht->begin();
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Temp:  Loading");
  lcd.setCursor(0, 1);
  lcd.print("Humid: Loading");
}

void loop() {
  // Read gracefully every 3 seconds without blocking the chip execution
  if (millis() - lastCheck >= 3000) {
    lastCheck = millis();
    
    if (dht != nullptr) {
      float humidity = dht->readHumidity();
      float temperature = dht->readTemperature();

      if (isnan(humidity) || isnan(temperature)) {
        lcd.setCursor(7, 0);
        lcd.print("Error   ");
        lcd.setCursor(7, 1);
        lcd.print("Sensor  ");
        Serial.println("Sensor packet dropped. Verify data line is secure on Pin 5.");
      } else {
        // Line 1 Update
        lcd.setCursor(7, 0);
        lcd.print(temperature, 1);
        lcd.write(223); // Standard degrees symbol
        lcd.print("C   ");
        
        // Line 2 Update
        lcd.setCursor(7, 1);
        lcd.print(humidity, 1);
        lcd.print("%   ");
        
        Serial.print("T: "); Serial.print(temperature); Serial.print(" | H: "); Serial.println(humidity);
      }
    }
  }
}
