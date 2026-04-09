// Written by Benjamin Wenger on 4-9-26
// Last revision 4-9-26
// Functionality test code for I2C sensors

/*
General I2C Wiring Instructions:
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

// One is true if a verdict has been passed on a sensor
bool printedFail = false;
bool printedSucc = false;

// True if all sensors have been tested
bool allTested = false;

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
}

// =========================== FINITE STATE MACHINE =========================== //
void loop() {
  // put your main code here, to run repeatedly:

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

// Read from ADS1115
void adsRead() {
  adc0 = ads.readADC_SingleEnded(0); // Read raw value from channel 0

  voltage = (adc0 * multiplier) / 1000; // Convert raw value to voltage (V)
  // Keep the below lines for debugging
  // Serial.print("\tVoltage: ");
  // Serial.print(voltage, 4); // Print voltage with 4 decimal places
  // Serial.println("V");
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