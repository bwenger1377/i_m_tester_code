// Written by Benjamin Wenger on 4-7-26
// Last revision 4-7-26
// Functionality Test Code for Adafruit ADS1115 16-Bit Analog-to-Digital Converter

// Include necessary libraries
#include <Wire.h>
#include <Adafruit_ADS1X15.h>

Adafruit_ADS1115 ads;     // Create an ADS1115 object with default I2C address 0x48
const float multiplier = 0.1875F; // Multiplier to convert raw 16-bit value to voltage (mV) at default gain

// State of loop operations
enum State {WAITING, READING, VERDICT};
State status = WAITING;

// Variables to indicate if sensor works (default is yes)
bool is_working = true;
bool printedFail = false;
bool printedSucc = false;

int16_t adc0;  // 16-bit ADC read variable
float voltage = 0; // Voltage reading from ADC

void setup() {
  // Begin serial communication
  Serial.begin(115200);
  while (!Serial) delay(1000); // Wait until serial monitor is opened
  delay(3000); // 3-second delay to give the user some time to prepare

  // Initialize the ADS1115
  ads.begin();
  if (!ads.begin()) {
    Serial.println("ADS1115 not found.");
  }
  Serial.println("ADS1115 initialized.");

  Serial.println("Press SPACE + ENTER to begin testing.");
}

void loop() {
  // Read from the ADS1115
  adsread();
  
  delay(1000); // Wait for a second before the next reading
}

void adsread() {
  adc0 = ads.readADC_SingleEnded(0); // Read the value from channel 0

  Serial.print("AIN0 Raw Value: ");
  Serial.print(adc0);
  // Convert raw value to voltage in Volts
  voltage = (adc0 * multiplier) / 1000;
  Serial.print("\tVoltage: ");
  Serial.print(voltage, 4); // Print voltage with 4 decimal places
  Serial.println("V");
}