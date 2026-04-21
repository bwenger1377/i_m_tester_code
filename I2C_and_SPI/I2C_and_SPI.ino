// Written by Benjamin Wenger on 4-20-26
// Last revision 4-20-26
// Functionality test code for I2C and SPI sensors

/*
General I2C Sensor Wiring Instructions:
1. Vin --> Arduino 5V
2. GND --> Arduino GND
3. SDA --> Arduino SDA
4. SCL --> Arduino SCL

General SPI Wiring Instructions:
1. Vin --> Arduino 5V
2. GND --> Arduino GND
3. CS --> Arduino D10
4. MOSI --> Arduino D11
5. MISO --> Arduino D12
6. SCLK --> Arduino D13

Sensors:
1. Adafruit ADS1115 16-bit Analog-to-Digital Converter (ADS)
  a) Wiring instructions
    - GND --> Power Supply GND also
    - A0 --> Arduino A0 pin & Power Supply as instructed
  b) Related functions:
    - adsInitialize()
    - adsRead()
2. Adafruit LIS3DH 3-axis Accelerometer (LIS)
  a) Wiring instructions:
    - None for this sensor
  b) Related functions:
    - lisInitialize()
    - lisRead()
    - updateMax()
    - lisVerdict()
3. Adafruit VL53L1X Flight Distance Sensor (VLX)
  a) Wiring instructions:
    - IRQ_PIN --> Arduino digital pin
    - XSHUT_PIN --> Arduino digital pin
  b) Related functions:
    - vlxInitialize()
    - vlxRead()
4. Adafruit SD Card Breakout Board + (BREAKOUT)
  a) Wiring instructions: 
    - See General SPI Wiring Instructions
  b) Related functions:
    - sdInitialize()
  c) Additional notes:
    - Assume FAT32 format (anything else is outside the scope of this test)
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

// --------- SD Card Breakout Board + --------- //
#include <SPI.h>
#include <SD.h>

// =========================== VARIABLES =========================== //

// --------- GENERAL --------- //

// State of loop operations
enum State {WAITING,READING,VERDICT};
State status = WAITING;

char c; // Character inputted to the serial monitor by the user

// True if sensor is working
bool is_working = true;

// True if all desired sensors for the current test run have been initialized
bool all_init = false;

// List of sensors to be tested in current test run
bool to_test[5] = {false,false,false,false,false}; // for each sensor in the list, 0 means it will not be tested; during initialization, desired sensors will be set to 1

// One is true if a verdict has been passed on a sensor
bool printedFail = false;
bool printedSucc = false;

// Sensor selection
enum Select {ADS1115,LIS3DH,VL53L1X,BREAKOUT,PEC11};
Select sensor = ADS1115;

// True if all desired sensors have been tested
bool all_tested = false;

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

// --------- SD Card Breakout Board + --------- //

// CS Pin assignment
const int chipSelect = 10;

// --------- PEC11 --------- //

// Direction of rotation
enum Direction {CW,CCW};
Direction rotDir = CW;

// Pin locations
const int enc_bottom_pin = 2; // Encoder pin that is not the middle
const int enc_top_pin = 3; // Encoder pin that is not the middle (the other one)

// Variables needed to read rotational encoder values
int Encoded = 0; // Current value of general rotation
long LastEncoderValue = 0; // Last value of the encoder
int bin_sum = 0; // Sum of binary sequence
volatile int LastEncoded = 0; // Last value of the general rotation
volatile long EncoderValue = 0; // Current value of the encoder
int enc_bottom_val; // Reading from enc_bottom_pin
int enc_top_val; // Reading from enc_top_pin

// =========================== SETUP =========================== //
void setup() {
  // Begin serial communication
  Serial.begin(115200);
  while (!Serial) delay(1000); // Wait until serial monitor is opened
  delay(3000); // 3-second delay to give the user some time to prepare

  while (!all_init) {
    Serial.print("Would you like to test the "); Serial.print(sensor); Serial.println(" in this test run? (y/n): ");
    c = charPressed();
    if (c == 'y') { 
      to_test[sensor] = 1;
      switch (sensor) {
        case ADS1115: 
          if (!adsInitialize()) {
            Serial.print(sensor); Serial.println(" failed to initialize. Check connections, then try again.");
            while(1); 
          }
          break;
        case LIS3DH: 
          lisInitialize();
          break;
        case VL53L1X: 
          vlxInitialize();
          break;
        case BREAKOUT:
          sdInitialize();
          break;
        case PEC11:
          pecInitialize();
          break;
      }
      Serial.print(sensor); Serial.println(" initialized.");
      sensor = static_cast<Select>(sensor + 1);
    } else if (c == 'n') {
      sensor = static_cast<Select>(sensor + 1);
    }
    if (sensor > BREAKOUT) {
      all_init = true; // This will end the while loop
    }
  }

  nextSensor(); // Set the sensor to be tested

  // Prompt user input to begin testing
  Serial.println("Press SPACE + ENTER to begin testing.");
}

// =========================== FINITE STATE MACHINE =========================== //
void loop() {
  c = charPressed();
  if (!all_tested) { // If there are still sensors left to test
    if (!printedFail) { // If we have not already given a verdict on the current sensor
      // Run the waiting/reading/verdict FSM for this sensor with all necessary functions
      switch (status) {
        case WAITING: // waiting for user input
          if (c == ' ') {
            status = READING; // will execute code in second case next time loop repeats
            switch (sensor) {
              // Provide instructions for sensor testing and prompt the user to begin the test
              case ADS1115: adsPrompt(); break;
              case LIS3DH: lisPrompt(); break;
              case VL53L1X: vlxPrompt(); break;
              case PEC11: pecPrompt(); break;
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
              if (c == ' ') {
                adsDecide();
              }
              break;
            case LIS3DH:
              // Read from the LIS3DH
              lisRead();

              // Update max values for x, y, and z measurements
              lisUpdateMax();

              if (c == ' ') {
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

              if (c == ' ') {
                vlxDecide();
              }
              break;
            case BREAKOUT:
              sdTest(); // Test the sensor
              sdDecide(); // Determine whether the sensor is working
              break;
            case PEC11:
              pecDecide(); // Do not need to expressly read the sensor (done through interrupts), just decide here if it works
              break;
          }
          break;
        case VERDICT:
          // Print whether or not the sensor is working
          giveVerdict();
          nextSensor();
          if (!all_tested) {
            Serial.println("Would you like to test the next sensor? (y/n): ");
          }
          break;
      }
    } else if (c == 'y') {
      // Reset important loop logic for next sensor test
      reset4next();
      
      Serial.println("Press SPACE + ENTER to begin testing.");
    } else if (c == 'n') {
      all_tested = true; // Will print goodbye message on the next loop iteration
    }
  } else { // If all of the sensors have been tested
    Serial.println("All tests completed. Push reset button to run another round of tests.");
    while(1); // Hold indefinitely while the Mega is powered
  }
}

// =========================== RELATED FUNCTIONS =========================== //

// --------- GENERAL --------- //

// Determine what key a user pressed
char charPressed() {
  if(Serial.available()) {
    char c = Serial.read();
    while (Serial.available()) {
      Serial.read(); // clear buffer
    }
    return c;
  }
}

// Determine which sensor is next from the list of sensors to test
void nextSensor() {
  all_tested = true;
  // Iterate through the to_test array until we find a sensor that will be tested
  for (int ii = 0; ii < 5; ii++) {
    if (to_test[ii] == true) {
      sensor = static_cast<Select>(ii); // ii should correspond to the index of the sensor enum (ex. 0 corresponds to ADS1115)
      to_test[ii] = false; // eliminates repeated tests of the same sensor
      all_tested = false; // if we find that there's a sensor we haven't tested, set this false so that we actually test it
      break;
    }
  }
  all_tested = true; // will only execute if no more entries in to_test are true
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

// Function that resets important loop logic for next sensor
void reset4next() {
  is_working = true;
  printedFail = false;
  printedSucc = false;
  status = WAITING;
  level = POS;
  direction = X;
  proximity = NEAR;
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

// --------- SD Card Breakout Board + --------- //

// Initialize SD Card Breakout Board
void sdInitialize() {
  Serial.print("Initializing SD card... ");
  pinMode(chipSelect, OUTPUT); // Set the CS pin (Arduino 10) as an output from the Mega

  // Alert user if sensor fails to connect
  if (!SD.begin(chipSelect)) {
    Serial.println("initialization failed!");
    is_working = false;
    while(1); // REMOVE THIS IN MERGED CODE FILES
  }

  Serial.println("done.");
}

// Test SD Card Breakout Board
void sdTest() {
  // Open or create a file in the working directory
  File myFile = SD.open("test.txt", FILE_WRITE);

  // Write to a file on the SD card from the sensor
  if (myFile) {
    Serial.print("Writing to test.txt...");
    myFile.println("Hello from SD card!");
    myFile.close();  // very important!
    Serial.println("done.");
  } else {
    Serial.println("Error opening test.txt.");
  }

  // Read from the file on the SD card to the sensor
  myFile = SD.open("test.txt");
  if (myFile) {
    Serial.println("Content of test.txt:");
    while (myFile.available()) {
      Serial.write(myFile.read());
    }
    myFile.close();
  } else {
    Serial.println("Error opening test.txt.");
  }
  Serial.println("If content of test.txt is Hello from SD card! then the sensor is working.");
}

// Determine (with user input) whether the sensor is working
void sdDecide() {
  Serial.println("Is the sensor working? (y/n): ");

  // Wait to move on until the user has confirmed whether or not the sensor is working
  while (c != 'y' || c != 'n') {
    c = charPressed();
  }

  if (c == 'n') {
    is_working = false;
  }
  status = VERDICT;
}

// --------- PEC11 --------- //

// Initialize encoder
void pecInitialize() {
  Serial.println("Do not touch encoder. Initializing...");

  // Setting up input types for pins on encoder
  pinMode(enc_bottom_pin,INPUT);        //One of the encoder pins and set to HIGH
    digitalWrite(enc_bottom_pin,HIGH);
  pinMode(enc_top_pin,INPUT);           //The other encoder pin and set to HIGH
    digitalWrite(enc_top_pin,HIGH);

  // Set interrupt routine, so the UpdateEncoder() function is activated when any high/low
  // change is seen on interrupt 0 (bottom pin), or interrupt 1 (top pin).
  attachInterrupt(0,UpdateEncoder,CHANGE);
  attachInterrupt(1,UpdateEncoder,CHANGE);
}

// Provide instructions to user for testing
void pecPrompt() {
  if (rotDir == CW) { // prompt the user based on the direction of encoder rotation
    Serial.println("Beginning test. Slowly rotate the encoder a few clicks clockwise, then press SPACE + ENTER to conclude test.");
  } else {
    Serial.println("Beginning test. Slowly rotate the encoder a few clicks counterclockwise, then press SPACE + ENTER to conclude test.");
  }
}

// Function to read the encoder
void UpdateEncoder() {

  // Reading digital pins (left side of rotary encoder)
  enc_bottom_val = digitalRead(enc_bottom_pin);
  enc_top_val = digitalRead(enc_top_pin);

  // Converting reading and sum to binary sequences
  Encoded = (enc_top_val << 1) | enc_bottom_val;
  bin_sum = (LastEncoded << 2) | Encoded;

  // Deciding if increasing or decreasing in rotations
  if(bin_sum == 0b0001 || bin_sum == 0b0111 || bin_sum == 0b1110 || bin_sum == 0b1000)
  {
    EncoderValue ++;
  }
  if(bin_sum == 0b0010 || bin_sum == 0b1011 || bin_sum == 0b1101 || bin_sum == 0b0100)
  {
    EncoderValue --;
  }
  // Updating variables
  LastEncoded = Encoded;
}

// Decide if the encoder is working
void pecDecide() {
  if (rotDir == CW) {
    if (c == ' ') {
      status = WAITING;
      rotDir = CCW;
      Serial.println("Test complete.");
      if (!Encoded || EncoderValue <= 0) {
        is_working = false;
        status = VERDICT;
      }
    }
  } else {
    if (c == ' ') {
      status = VERDICT;
      Serial.println("Test complete.");
      if (!Encoded || EncoderValue >= 0) {
        is_working = false;
      }
    }
  }
}