// Written by Benjamin Wenger on 2-24-2026
// Last revision 4-21-2026
// Functionality test code for Adafruit ADXL335 3-axis accelerometer

// Confirm that significant movements generate different results for the x, y, and z components. 
// If the code does not see significant changes after prompting movements, it should tell the user that the sensor is defective.

const int xPin = A5;
const int yPin = A4;
const int zPin = A3;

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
  // Read initial acceleration values and zero out the sensor
  Serial.println("Measuring initial accelerations. Keep board on a flat surface...");
  delay(3000); // Give the user a chance to set the board down
  for (int ii = 0; ii < 400; ii++) {
    // Get a new sensor reading with normalized accelerations
    adxlRead();

    Serial.println(vals[0]); // for debugging
    
    // Decide whether sensor is working based on offsets. x and y should be close to 0; z should be close to 9.81.
    if (fabs(vals[0]) > 1.0 || fabs(vals[1]) > 1.0 || vals[2] > 11.0 || vals[2] < 7.0) {
      Serial.println("Sensor measurements do not match expected values.");
      is_working = false;
      status = VERDICT;
      break;
    }
    // Short delay
    delay(1);
  }
  Serial.println("Press SPACE + ENTER to begin testing.");
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
              Serial.println("Move shield rapidly back and forth in the X-direction marked on the sensor.\nWhen you are finished, press SPACE + ENTER");
              break;
            case Y: // y-direction
              Serial.println("Move shield rapidly back and forth in the Y-direction marked on the sensor.\nWhen you are finished, press SPACE + ENTER");
              break;
            case Z: // z-direction
              Serial.println("Move shield rapidly back and forth in the Z-direction marked on the sensor.\nWhen you are finished, press SPACE + ENTER");
              break;
          }
        }
        break;
      case READING:
        // Read and log current sensor readings
        adxlRead();

        // Update max values for x, y, and z measurements
        updateMax();
        
        if (direction == Z) {
          if (spacePressed()) {
            //Serial.println(fabs(maxVals[direction]));
            status = VERDICT;
            Serial.println("Testing complete.");
            adxlVerdict();
          }
        } else {
          if (spacePressed()) {
            //Serial.println(fabs(maxVals[direction]));
            status = WAITING;
            adxlVerdict();
            direction = (Axis)(direction + 1);
            if (is_working == true) {
              Serial.println("Test complete. Press SPACE + ENTER to begin next test.");
            }
          }
        }
        break;
      case VERDICT:
        if (!printedSucc) {
          if (is_working) {
            Serial.println("ADXL335 accelerometer is working.");
            printedSucc = true;
          } else {
            Serial.println("ADXL335 accelerometer is not working.");
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
    return Serial.read() == ' ';
  }
  return false;
}

// Function for reading from LIS3DH accelerometer
void adxlRead() {
  // Read raw analog values
  int rawX = analogRead(xPin);
  int rawY = analogRead(yPin);
  int rawZ = analogRead(zPin);

  // Convert to voltage
  float xVolt = map(rawX, 0,1023,0,5000);
  float yVolt = map(rawY, 0,1023,0,5000);
  float zVolt = map(rawZ, 0,1023,0,5000);

  // Convert voltage to acceleration in g
  vals[0] = 9.81*(map(xVolt, 0, 3300, -5000, 5000)/1000.0);
  vals[1] = 9.81*(map(yVolt, 0, 3300, -5000, 5000)/1000.0);
  vals[2] = 9.81*(map(zVolt, 0, 3300, -5000, 5000)/1000.0);
}

// Function that updates max values in a given direction
void updateMax() {
  if (vals[direction] > maxVals[direction]) {
    maxVals[direction] = vals[direction];
  }
}

// Function that decides whether the sensor is working
void adxlVerdict() {
  if (direction == Z) {
    if (fabs(maxVals[Z]) < 2.0) {
      is_working = false;
    }
  } else {
      if (fabs(maxVals[direction]) < 2.0) {
      is_working = false;
      status = VERDICT;
    }
  }
}