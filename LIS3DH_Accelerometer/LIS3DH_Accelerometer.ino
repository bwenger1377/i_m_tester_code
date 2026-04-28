// Written by Benjamin Wenger on 2-24-2026
// Last modified 3-30-2026
// Functionality test code for Adafruit LIS3DH 3-axis accelerometer

// Confirm that significant movements generate different results for the x, y, and z components. 
// If the code does not see significant changes after prompting movements, it should tell the user that the sensor is defective.

#include <Wire.h>
#include <Adafruit_LIS3DH.h>
#include <Adafruit_Sensor.h>

// I2C address
Adafruit_LIS3DH lis = Adafruit_LIS3DH();

// Sensor reading variables
float vals[3] = {0,0,0}; // x,y,z
float maxVals[3] = {0,0,0}; // x_max,y_max,z_max

// State of loop operations
enum State {WAITING, READING, VERDICT};
State status = WAITING;

// Active measurement axis
enum Axis {X, Y, Z};
Axis direction = X;

// Variables to indicate if sensor works (default is yes)
bool is_working = true;
bool printedFail = false;
bool printedSucc = false;

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  while (!Serial) delay(1000); // Wait until serial monitor is opened
  delay(3000); // 5-second delay to give the user some time to prepare

  lisInitialize();

  // Read initial acceleration values and zero out the sensor
  Serial.println("Measuring initial accelerations. Keep board on a flat surface...");
  delay(3000); // Give the user a chance to set the board down
  for (int ii = 0; ii < 400; ii++) {
    // Get a new sensor reading with normalized accelerations
    lisRead();

    // Serial.println(vals[0]);
    // Serial.println(vals[1]);
    // Serial.println(vals[2]); // for debugging
    
    // Decide whether sensor is working based on offsets. x and y should be close to 0; z should be close to 9.81.
    if (fabs(vals[0]) > 2.0 || fabs(vals[1]) > 2.0 || vals[2] > 11.0 || vals[2] < 7.0) {
      Serial.println("Sensor measurements do not match expected values.");
      is_working = false;
      status = VERDICT;
      break;
    }
    // Short delay
    delay(1);
  }
  if (is_working) {
    Serial.println("Press SPACE + ENTER to begin testing.");
  }
}

void loop() {
  if (!printedFail) {
    switch (status) {
      case WAITING:
        if (spacePressed()) {
          status = READING;
          switch (direction) {
            // Test x,y,z directions in that order to ensure all are working.
            case X: // x-direction
              Serial.println("Move shield rapidly forward and backward, then press SPACE + ENTER to conclude testing.");
              break;
            case Y: // y-direction
              Serial.println("Move shield rapidly from left to right, then press SPACE + ENTER to conclude testing.");
              break;
            case Z: // z-direction
              Serial.println("Move board rapidly upward and downward, then press SPACE + ENTER to conclude testing.");
              break;
          }
        }
        break;
      case READING:
        // Read and log current sensor readings
        lisRead();

        // Update max values for x, y, and z measurements
        lisUpdateMax();

        if (spacePressed()) {
          if (direction == Z) {
            status = VERDICT;
            Serial.println("Testing complete.");
            lisVerdict();
          } else {
            status = WAITING;
            Serial.println("Test complete. Press SPACE + ENTER to begin next test.");
            lisVerdict();
            direction = (Axis)(direction + 1);
          }
        }  
        break;
      case VERDICT:
        if (!printedSucc) {
          if (is_working) {
            Serial.println("LIS3DH accelerometer is working.");
            printedSucc = true;
          } else {
            Serial.println("LIS3DH accelerometer is not working.");
            printedFail = true;
          }
        }
        break;
    }
  }
}

// Helper function to determine if spacebar was pressed
bool spacePressed() {
  if (Serial.available()) {
    char c = Serial.read();
    while (Serial.available()) {
      Serial.read();
    }
    return c == ' ';
  }
  return false;
}

void lisInitialize() {
  // Begin I2C communication
  if (! lis.begin(0x18)) {   // change this to 0x19 for alternative i2c address
    Serial.println("LIS3DH not recognized.");
    while (1) yield();
  }
  Serial.println("LIS3DH found.");

  // lis.setRange(LIS3DH_RANGE_4_G);   // 2, 4, 8 or 16 G!
  Serial.print("Range = "); Serial.print(2 << lis.getRange());
  Serial.println("G");
}

// Function for reading from LIS3DH accelerometer
void lisRead() {
  sensors_event_t event;
  lis.getEvent(&event);
  vals[X] = event.acceleration.x;
  vals[Y] = event.acceleration.y;
  vals[Z] = event.acceleration.z;
}

// Function that updates max values in a given direction
void lisUpdateMax() {
  if (vals[direction] > maxVals[direction]) {
    maxVals[direction] = vals[direction];
  }
}

// Function that decides whether the sensor is working
void lisVerdict() {
  if (direction == Z) {
    if (abs(maxVals[Z]) < 11.0) {
      is_working = false;
      status = VERDICT;
    }
  } else {
    if (abs(maxVals[direction]) < 2.0) {
      is_working = false;
      status = VERDICT;
    }
  }
}