// Written by Benjamin Wenger on 4-9-26
// Last revision 4-15-26
// Functionality test code for I2C sensors

/*
General I2C Sensor Wiring Instructions:
1. Vin --> Arduino 5V
2. GND --> Arduino GND
3. SDA --> Arduino SDA
4. SCL --> Arduino SCL

Sensors:
1. ADS1115 16-bit Analog-to-Digital Converter (ADS)
  a) Wiring instructions
    - GND --> Power Supply GND also
    - A0 --> Arduino A0 pin & Power Supply as instructed
  b) Related functions:
    - adsInitialize()
    - adsRead()
2. LIS3DH 3-axis Accelerometer (LIS)
  a) Wiring instructions:
    - None for this sensor
  b) Related functions:
    - lisInitialize()
    - lisRead()
    - updateMax()
    - lisVerdict()
3. VL53L1X Flight Distance Sensor (VLX)
  a) Wiring instructions:
    - IRQ_PIN --> Arduino digital pin
    - XSHUT_PIN --> Arduino digital pin
  b) Related functions:
    - vlxInitialize()
    - vlxRead()
*/

// =========================== LIBRARIES =========================== //

// --------- GENERAL --------- //
#include <Wire.h>

// --------- ADS1115 --------- //
#include <Adafruit_ADS1X15.h>

// --------- LIS3DH --------- //
#include <Adafruit_LIS3DH.h>
#include <Adafruit_Sensor.h>

// --------- VL53L1X --------- //
#include <Adafruit_VL53L1X.h>

// =========================== VARIABLES =========================== //

// --------- GENERAL --------- //

// State of loop operations
enum State {WAITING,READING,VERDICT};
State status = WAITING;

// True if sensor is working
bool is_working = true;

// True if all desired sensors for the current test run have been initialized
bool all_init = false;

// One is true if a verdict has been passed on a sensor
bool printedFail = false;
bool printedSucc = false;

// Sensor selection
enum Select {ADS1115,LIS3DH,VL53L1X};
Select sensor = ADS1115;

// --------- ADS1115 --------- //

// ADS1115 object (default I2C address 0x48)
Adafruit_ADS1115 ads;

// Multiplier (raw 16-bit measurement to voltage)
const float multiplier = 0.1875F;

// Voltage setting
enum Voltage {POS,NEG};
Voltage level = POS;

// Raw 16-bit ADC reading
int16_t adc0;

// Converted ADC reading (up to +/- 5.4V)
float voltage = 0;

// Voltage reading from Arduino A0 pin (0-5V)
float ardVoltage = 0;

// Thresholds for decision logic
const float posMin = 5.05; // V
const float negMax = -0.1; // V

// --------- LIS3DH --------- //

// LIS3DH object (default I2C address 0x18)
Adafruit_LIS3DH lis = Adafruit_LIS3DH();

// Sensor reading variables
float vals[3] = {0,0,0}; // x,y,z
float maxVals[3] = {0,0,0}; // x_max,y_max,z_max

// Active measurement axis
enum Axis {X, Y, Z};
Axis direction = X;

// --------- VL53L1X --------- //

// Additional pin assignments
#define IRQ_PIN 4
#define XSHUT_PIN 5

// VL53L1X object (default I2C address 0x29)
Adafruit_VL53L1X vlx = Adafruit_VL53L1X(XSHUT_PIN, IRQ_PIN);

// Proximity setting
enum Proximity {NEAR,FAR};
Proximity proximity = NEAR;

// Distance measurement from VL53L1X (in mm)
int16_t distance;

// =========================== SETUP =========================== //
void setup() {
  // Begin serial communication
  Serial.begin(115200);
  while (!Serial) delay(1000); // Wait until serial monitor is opened
  delay(3000); // 3-second delay to give the user some time to prepare

  if (!all_init) {
    Serial.print("Would you like to test the "); Serial.print(sensor); Serial.println("in this test run? (y/n): ");
    if (charPressed() == 'y') {
      switch (sensor) {
        case ADS1115: 
          adsInitialize();
          break;
        case LIS3DH: 
          lisInitialize();
          break;
        case VL53L1X: 
          vlxInitialize();
          break;
      }
    }
  }

  /*
  if not all of the sensors have been initialized
    Ask user whether they would like to test the sensor in question
    if yes
      initialize that sensor
      if the sensor doesn't initialize
        prompt user to unplug arduino and check connections then try again
        infinite loop of doing nothing
      otherwise
        tell them the sensor is working
        add the sensor to a running list of sensors that will be tested during the run
    increment the sensor enum (regardless of whether answer is yes or no)
  otherwise
    set the current sensor being tested to the first one in the list of ones the user desires to test
  */

  // Prompt user input to begin testing
  Serial.println("Press SPACE + ENTER to begin testing.");
}

// =========================== FINITE STATE MACHINE =========================== //
void loop() {
  if (!printedFail) {
    // run the waiting/reading/verdict loop for this sensor with all necessary functions
    switch (status) {
      case WAITING: // waiting for user input
        if (spacePressed()) {
          status = READING; // will execute code in second case next time loop repeats
          switch (sensor) {
            // Instruct user on testing process
            case ADS1115: adsPrompt(); break;
            case LIS3DH: lisPrompt(); break;
            case VL53L1X: vlxPrompt(); break;
          }
        }
        break;
      case READING:
        switch (sensor) {
          case ADS1115:
            // Read from the ADS1115
            adsRead();

            // Read from the Arduino analog pin for comparison
            ardVoltage = analogRead(A0) * (5.0f / 1023.0f); // Multiplier converts raw value to voltage

            // Determine whether the sensor is working
            if (spacePressed()) {
              adsDecide();
            }
            break;
          case LIS3DH:
            // Read from the LIS3DH
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
          case VL53L1X:
            // Read from the VL53L1X
            vlxRead();

            if (spacePressed()) {
              vlxDecide();
            }
            break;
        }
        break;
      case VERDICT:
        // Print whether or not the sensor is working
        giveVerdict();
        break;
    }
  } else {
    // ask user if they'd like to test another sensor
    if (charPressed() == 'y') {
      printedFail = 0;
      printedSucc = 0;
      // ask which sensor they'd like to test
      // if or case statement here
        // adjust value of sensor enum as needed
    }
  }
}

// =========================== RELATED FUNCTIONS =========================== //

// --------- GENERAL --------- //

// Determine if spacebar pressed
bool spacePressed() {
  if (Serial.available()) {
    char c = Serial.read();
    while (Serial.available()) {
      Serial.read(); // clear buffer
    }
    return c == ' '; // true if the character read & stored is a space
  }
  return false;
}

// Determine what key a user pressed
char charPressed() {
  if(Serial.available()) {
    char c = Serial.read();
    while (Serial.available()) {
      Serial.read(); // clear buffer
    }
    return c;
  }
  return false;
}

// Give a verdict on sensor functionality
void giveVerdict() {
  if (!printedSucc) {
    if (is_working) {
      Serial.print(sensor);
      Serial.println(" is working.");
      printedSucc = true;
    } else {
      Serial.print(sensor);
      Serial.println(" is not working.");
      printedFail = true;
    }
  }
}

// --------- ADS1115 --------- //

// Initialize ADS1115
bool adsInitialize() {
  if (!ads.begin()) {
    Serial.println("ADS1115 not found.");
    return false;
  }
  Serial.println("ADS1115 initialized.");
  return true;
}

// Instruct user on how to test ADS1115
void adsPrompt() {
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

// Read from ADS1115
void adsRead() {
  adc0 = ads.readADC_SingleEnded(0); // Read raw value from channel 0

  voltage = (adc0 * multiplier) / 1000; // Convert raw value to voltage (V)
  // Keep the below lines for debugging
  // Serial.print("\tVoltage: ");
  // Serial.print(voltage, 4); // Print voltage with 4 decimal places
  // Serial.println("V");
}

// Decide if ADS1115 is working
void adsDecide() {
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

// --------- LIS3DH --------- //

// Initialize LIS3DH
void lisInitialize() {
  // Begin I2C communication; wait until sensor recognized
  if (! lis.begin(0x18)) {   // change to 0x19 for alternative i2c address
    Serial.println("LIS3DH not recognized.");
    while (1) yield();
  }
  Serial.println("LIS3DH found.");

  // lis.setRange(LIS3DH_RANGE_4_G);   // 2, 4, 8 or 16 G!
  Serial.print("Range = "); Serial.print(2 << lis.getRange());
  Serial.println("G");
}

void lisPrompt() {
  switch (direction) {
  // Test x,y,z directions in that order to ensure all are working.
  case X: // x-direction
    Serial.println("Move shield rapidly forward and backward.");
    break;
  case Y: // y-direction
    Serial.println("Move shield rapidly from left to right.");
    break;
  case Z: // z-direction
    Serial.println("Move board rapidly upward and downward.");
    break;
  }
}


// Read from LIS3DH
void lisRead() {
  sensors_event_t event;
  lis.getEvent(&event);
  vals[X] = event.acceleration.x;
  vals[Y] = event.acceleration.y;
  vals[Z] = event.acceleration.z;
}

// Update max values in the current direction
void lisUpdateMax() {
  if (vals[direction] > maxVals[direction]) {
    maxVals[direction] = vals[direction];
  }
}

// Decide whether LIS3DH is working
void lisVerdict() {
  if (direction == Z) {
    if (abs(maxVals[Z]) < 10.0) {
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

// --------- VL53L1X --------- //

// Initialize VL53L1X
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

void vlxPrompt() {
  if (proximity == NEAR) {
    Serial.println("Beginning test. Place hand 6 inches above sensor, then press SPACE + ENTER to conclude test.");
  } else {
    Serial.println("Beginning test. Remove hand from above sensor, then press SPACE + ENTER to conclude test.");
  }
}

// Read from VL53L1X
void vlxRead() {
  if (vlx.dataReady()) {
    // new measurement for the taking!
    distance = vlx.distance();
    if (distance == -1) {
      // something went wrong!
      Serial.print(F("Couldn't get distance: "));
      Serial.println(vlx.vl_status);
      return;
    }
    // Serial.print(F("Distance: "));
    // Serial.print(distance);
    // Serial.println(" mm");

    // data is read out, time for another reading!
    vlx.clearInterrupt();
  }
}

void vlxDecide() {
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