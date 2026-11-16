#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Initialize the 16x2 LCD at I2C address 0x27
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Pin Configurations
const int sensorPin = A0;

// Timing Variables
unsigned long lastBeatTime = 0;
const int samplePeriod = 20; // Sample every 20ms (50Hz)
unsigned long lastSampleTime = 0;

// Signal Variables
int signalValue = 0;
int maxSignal = 0;
int minSignal = 1023;
int threshold = 850;   // Dynamic default, will overwrite during calibration
bool pulseDetected = false;
int bpm = 0;

// Custom heart icon byte array
byte heartIcon[] = {
  B00000, B01010, B11111, B11111, B11111, B01110, B00100, B00000
};

void setup() {
  Serial.begin(115200);
  
  lcd.init();
  lcd.backlight();
  lcd.createChar(0, heartIcon);
  
  // 1. CALIBRATION PHASE: Run for the first 5 seconds
  lcd.setCursor(0, 0);
  lcd.print("Place Finger... ");
  lcd.setCursor(0, 1);
  lcd.print("Calibrating: 5s ");
  
  unsigned long calibrationStart = millis();
  while (millis() - calibrationStart < 5000) {
    int sensorRead = analogRead(sensorPin);
    if (sensorRead > maxSignal) maxSignal = sensorRead;
    if (sensorRead < minSignal && sensorRead > 200) minSignal = sensorRead; // Ignore 0 readings
    delay(20);
  }
  
  // Set the threshold exactly halfway between your finger's peak and valley readings
  threshold = (maxSignal + minSignal) / 2;
  
  // Set up standard display UI
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Heart Rate Mon.");
  lcd.setCursor(0, 1);
  lcd.print("BPM: ---");
  
  Serial.println("SignalValue,BPM");
}

void loop() {
  unsigned long currentTime = millis();
  
  if (currentTime - lastSampleTime >= samplePeriod) {
    lastSampleTime = currentTime;
    signalValue = analogRead(sensorPin);
    
    // Dynamic Peak Detection based on calibration values
    if (signalValue > threshold && !pulseDetected) {
      pulseDetected = true;
      
      unsigned long duration = currentTime - lastBeatTime;
      
      // Filter out double-triggers and impossible human heart rates
      if (duration > 400 && duration < 1500) { 
        bpm = 60000 / duration;
        
        lcd.setCursor(5, 1);
        lcd.print(bpm);
        lcd.print("   "); 
        
        lcd.setCursor(14, 1);
        lcd.write(0); 
      }
      lastBeatTime = currentTime;
    }
    
    // Hysteresis reset: Must drop down well below the threshold to reset the lock
    if (signalValue < (threshold - 40) && pulseDetected) {
      pulseDetected = false;
      lcd.setCursor(14, 1);
      lcd.print(" ");
    }

    // Output to Serial Monitor/Plotter
    Serial.print(signalValue);
    Serial.print(",");
    Serial.println(bpm);
  }
}
