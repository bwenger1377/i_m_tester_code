// Written by Benjamin Wenger on 4-7-26
// Last revision 4-8-26
// Functionality Test Code for Adafruit ADS1115 16-Bit Analog-to-Digital Converter

// Include necessary libraries
#include <Wire.h>
#include <Adafruit_ADS1X15.h>

Adafruit_ADS1115 ads;     // Create an ADS1115 object with default I2C address 0x48
const float multiplier = 0.1875F; // Multiplier to convert raw 16-bit value to voltage (mV) at default gain

// State of loop operations
enum State {WAITING, READING, VERDICT};
State status = WAITING;

// Voltage Setting
enum Voltage {POS,NEG};
Voltage level = POS;

// Variables to indicate if sensor works (default is yes)
bool is_working = true;
bool printedFail = false;
bool printedSucc = false;

int16_t adc0;  // 16-bit ADC read variable
float voltage = 0; // Voltage reading from ADC (+/- 5.4 V)
float ardVoltage = 0; // Voltage reading from Arduino ADC (0-5 V)
const float posMin = 5.05;
const float negMax = -0.1;

void setup() {
  // Begin serial communication
  Serial.begin(115200);
  while (!Serial) delay(1000); // Wait until serial monitor is opened
  delay(3000); // 3-second delay to give the user some time to prepare

  // Initialize the ADS1115
  if (!adsInitialize()) {
    while(1); // Stop any further code from executing
  }

  // Prompt user to begin testing
  Serial.println("Press SPACE + ENTER to begin testing.");
}

void loop() {
  // Only execute code in the loop if it is not known whether or not the sensor works
  if (!printedFail) {
    switch (status) {
      case WAITING: // waiting for user input
        if (spacePressed()) {
          status = READING; // will execute code in second case next time loop repeats
          if (level == POS) {
            Serial.println("Beginning test. Turn bench power supply on and set to 5.1 V. Then connect the positive lead to the A0 pin.");
            Serial.println("Press SPACE + ENTER to conclude test.");
          } else {
            Serial.println("Beginning test. First, turn off the power supply.");
            Serial.println("Connect the banana plug between the + and GND terminals of the bench power supply. Turn power supply back on and set to 0.5 V.");
            Serial.println("Then connect the grounded terminal to GND, and the - terminal of the power supply to the A0 pin on the ADS1115.");
            Serial.println("Press SPACE + ENTER to conclude test.");
          }
        }
        break;
      case READING:
        // Read from the ADS1115
        adsRead();

        // Read from the Arduino analog pin for comparison
        ardVoltage = analogRead(A0) * (5.0f / 1023.0f); // Multiplier converts raw value to voltage

        if (spacePressed()) {
          if (level == POS) {
            level = NEG;
            if (voltage < posMin) {
              if (ardVoltage <= 5.0) {
                is_working = false;
                status = VERDICT;
              }
              Serial.println(ardVoltage);
            } else {
              Serial.println("Test complete. Press SPACE + ENTER to begin next test.");
              status = WAITING;
            }
          } else {
            status = VERDICT;
            Serial.println("Test complete.");
            if ((voltage > negMax) && (ardVoltage <= 0.1)) {
              is_working = false;
            }
          }
        }
        break;
      case VERDICT:
        if (!printedSucc) {
          if (is_working) {
            Serial.println("ADS1115 is working.");
            printedSucc = true;
          } else {
            Serial.println("ADS1115 is not working.");
            printedFail = true;
          }
        }
        break;
    }
  }
}

// Function used to read from the ADS1115
void adsRead() {
  // Read raw value from channel 0
  adc0 = ads.readADC_SingleEnded(0);

  // Convert raw value to voltage in Volts
  voltage = (adc0 * multiplier) / 1000;
  // Serial.print("\tVoltage: ");
  // Serial.print(voltage, 4); // Print voltage with 4 decimal places
  // Serial.println("V");
}

// Function called to initialize the ADS1115
bool adsInitialize() {
  if (!ads.begin()) {
    Serial.println("ADS1115 not found.");
    return false;
  }
  Serial.println("ADS1115 initialized.");
  return true;
}

// Helper function to determine if spacebar was pressed
bool spacePressed() {
  if (Serial.available()) {
    char c = Serial.read();
    while (Serial.available()) {
      Serial.read(); // clear buffer
    }
    return c == ' '; // true if the first character read was a space (this function is returning a boolean)
  }
  return false;
}