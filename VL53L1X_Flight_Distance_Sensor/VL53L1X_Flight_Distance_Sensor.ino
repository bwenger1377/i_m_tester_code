// Written by Benjamin Wenger on 4-2-26
// Last revision 4-2-26
// VL53L1X Laser Distance Sensor - Functionality Test

// Include sensor library
#include <Adafruit_VL53L1X.h>

// Declare necessary additional pins for sensor function
#define IRQ_PIN 2
#define XSHUT_PIN 3

// Create an object for sensor communication via I2C protocol
Adafruit_VL53L1X vlx = Adafruit_VL53L1X(XSHUT_PIN, IRQ_PIN);

// State of loop operations
enum State {WAITING, READING, VERDICT};
State status = WAITING;

// Proximity setting
enum Proximity {NEAR,FAR};
Proximity proximity = NEAR;

// Variables to indicate if sensor works (default is yes)
bool is_working = true;
bool printedFail = false;
bool printedSucc = false;

// Distance measurement from VL53L1X (in mm)
int16_t distance;

void setup() {
  // Begin serial communication
  Serial.begin(115200);
  while (!Serial) delay(1000); // Wait until serial monitor is opened
  delay(3000); // 3-second delay to give the user some time to prepare

  vlxInitialize();

  Serial.println("Press SPACE + ENTER to begin testing.");
}

void loop() {
  // Only execute code in the loop if it is not known whether or not the sensor works
  if (!printedFail) {
    switch (status) {
      case WAITING: // waiting for user input
        if (spacePressed()) {
          status = READING; // will execute code in second case next time loop repeats
          if (proximity == NEAR) {
            Serial.println("Beginning test. Place hand 6 inches above sensor, then press SPACE + ENTER to conclude test.");
          } else {
            Serial.println("Beginning test. Remove hand from above sensor, then press SPACE + ENTER to conclude test.");
          }
        }
        break;
      case READING:
        vlxRead();

        if (spacePressed()) {
          if (proximity == NEAR) {
            proximity = FAR;
            if (distance >= 250 || distance <= 50) {
              is_working = false;
              status = VERDICT;
            } else {
              Serial.println("Test complete. Press SPACE + ENTER to begin next test.");
              status = WAITING;
            }
          } else {
            status = VERDICT;
            Serial.println("Test complete.");
            if (distance < 1000) {
              is_working = false;
            }
          }
        }
        break;
      case VERDICT:
        if (!printedSucc) {
          if (is_working) {
            Serial.println("VL53L1X is working.");
            printedSucc = true;
          } else {
            Serial.println("VL53L1X is not working.");
            printedFail = true;
          }
        }
        break;
    }
  }
}

void vlxInitialize() {
  // Initialize sensor communication via I2C
  Wire.begin();
  if (! vlx.begin(0x29, &Wire)) {
    Serial.print(F("Error on init of VL sensor: "));
    Serial.println(vlx.vl_status);
    while (1)       delay(10);
  }
  Serial.println(F("VL53L1X sensor OK!"));

  Serial.print(F("Sensor ID: 0x"));
  Serial.println(vlx.sensorID(), HEX);

  if (! vlx.startRanging()) {
    Serial.print(F("Couldn't start ranging: "));
    Serial.println(vlx.vl_status);
    while (1)       delay(10);
  }
  Serial.println(F("Ranging started"));

  // Valid timing budgets: 15, 20, 33, 50, 100, 200 and 500ms!
  vlx.setTimingBudget(50);
  Serial.print(F("Timing budget (ms): "));
  Serial.println(vlx.getTimingBudget());
}

void vlxRead() {
  if (vlx.dataReady()) {
    // new measurement for the taking!
    distance = vlx.distance();
    if (distance == -1) {
      return;
    }
    vlx.clearInterrupt();
  }
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