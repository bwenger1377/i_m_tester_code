// Written by Benjamin Wenger on 4-20-26
// Last revision 4-22-26
// Functionality test code for I2C and SPI sensors

/*
General I2C Sensor Wiring Instructions:
1. SDA --> Arduino SDA
2. SCL --> Arduino SCL

General SPI Wiring Instructions:
1. CS --> Arduino D10
2. MOSI --> Arduino D11
3. MISO --> Arduino D12
4. SCLK --> Arduino D13

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
    - accelPrompt
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
5. PEC11 Rotary Encoder (PEC)
  a) Wiring instructions:
    - There is a group of 3 pins and a group of 2 pins.
    - The middle pin in the group of 3 should be --> GND
    - Other two pins will go to pins 2 and 3 on the Mega (any interrupt-capable pins)
    - Solder board will make this easier to determine
  b) Related functions:
    - pecInitialize()
    - pecPrompt()
    - UpdateEncoder()
    - pecDecide()
6. SY-M213 Hall Effect Module (HALL)
  a) Wiring instructions:
    - D0 --> Arduino digital pin
  b) Related functions:
    - hallInitialize()
    - hallPrompt()
    - hallDecide()
7. HC-SR04 Ultrasonic Sensor (HCSR04)
  a) Wiring instructions:
    - Trig --> Arduino digital pin
    - Echo --> Arduino digital pin
  b) Related functions:
    - hcInitialize()
    - hcPrompt()
    - hcRead()
    - hcDecide()
8. ADXL335 3-Axis Accelerometer (ADXL335)
  a) Wiring instructions:
    - Xout, Yout, Zout --> Arduino analog pins
  b) Related functions:
    - adxlInitialize()
    - accelPrompt()
    - updateMax()
    - adxlRead()
    - adxlDecide()
    - adxlVerdict()
9. Runeskee Strain Gauge (STRAIN)
  a) Wiring instructions:
    - A0 --> Arduino analog pin
  b) Related functions:
    - strainPrompt()
    - strainDecide()
10. Modern Device Wind Sensor Rev C (WIND)
  a) Wiring instructions:
    - TMP --> Arduino analog pin
    - RV --> Arduino analog pin
  b) Related functions:
    - windInitialize()
    - windPrompt()
    - windRead()
    - windDecide()
11. HX711 Load Cell Amplifier (LOAD)
  a) Wiring instructions:
    - RED --> TAL220 red wire
    - BLK --> TAL220 black wire
    - WHT --> TAL220 white wire
    - GRN --> TAL220 green wire
    - VCC --> Arduino 5V
    - DAT --> Arduino digital pin
    - CLK --> Arduino digital pin
  b) Related functions:
    - loadInitialize()
    - loadPrompt()
    - loadRead()
    - loadDecide()
*/

// ============================================================================================================ LIBRARIES ============================================================================================================ //

// ------------------------------ GENERAL ------------------------------ //
#include <Wire.h>

// ------------------------------ ADS1115 ------------------------------ //
#include <Adafruit_ADS1X15.h>

// ------------------------------ LIS3DH ------------------------------ //
#include <Adafruit_LIS3DH.h>
#include <Adafruit_Sensor.h>

// ------------------------------ VL53L1X ------------------------------ //
#include <Adafruit_VL53L1X.h>

// ------------------------------ SD Card Breakout Board + ------------------------------ //
#include <SPI.h>
#include <SD.h>

// ------------------------------ LOAD ------------------------------ //

#include <Adafruit_HX711.h>

// ============================================================================================================ VARIABLES ============================================================================================================ //

// ------------------------------ GENERAL ------------------------------ //

// State of loop operations
enum State {WAITING,READING,VERDICT};
State status = WAITING;

char c = '\0'; // Character inputted to the serial monitor by the user

// True if sensor is working
bool is_working[11] = {true,true,true,true,true,true,true,true,true,true,true};

// True if all desired sensors for the current test run have been initialized
bool all_init = false;

// List of sensors to be tested in current test run
bool to_test[11] = {false,false,false,false,false,false,false,false,false,false,false}; // for each sensor in the list, false means it will not be tested; during initialization, desired sensors will be set to true
bool tested[11] = {false,false,false,false,false,false,false,false,false,false,false}; // used to print a summary of the sensors tested at the end of each run

// One is true if a verdict has been passed on a sensor
bool printedFail = false;
bool printedSucc = false;

// Sensor selection
enum Select {ADS1115,LIS3DH,VL53L1X,BREAKOUT,PEC11,HALL,HCSR04,ADXL335,STRAIN,WIND,LOAD};
Select sensor = ADS1115;

// True if all desired sensors have been tested
bool all_tested = false;

// ------------------------------ ADS1115 ------------------------------ //

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

// ------------------------------ LIS3DH ------------------------------ //

// LIS3DH object (default I2C address 0x18)
Adafruit_LIS3DH lis = Adafruit_LIS3DH();

// Sensor reading variables
float vals[3] = {0,0,0}; // x,y,z
float maxVals[3] = {0,0,0}; // x_max,y_max,z_max

// Active measurement axis
enum Axis {X, Y, Z};
Axis direction = X;

// ------------------------------ VL53L1X ------------------------------ //

// Additional pin assignments
#define IRQ_PIN 4
#define XSHUT_PIN 5

// VL53L1X object (default I2C address 0x29)
Adafruit_VL53L1X vlx = Adafruit_VL53L1X(XSHUT_PIN, IRQ_PIN);

// Proximity setting (will also be used by the HC-SR04)
enum Proximity {NEAR,FAR};
Proximity proximity = NEAR;

// Distance measurement from VL53L1X (in mm)
int16_t distance;

// ------------------------------ SD Card Breakout Board + ------------------------------ //

// CS Pin assignment
const int chipSelect = 53;

// ------------------------------ PEC11 ------------------------------ //

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

// ------------------------------ HALL ------------------------------ //

// Declare pin number
const int hallPin = 49;   // substitute for digital pin connected to SY-M213 output

// Sensor reading variables
int s_curr = 0; // Current sensor reading

// ------------------------------ HC-SR04 ------------------------------ //

// Pin assignments
const int trig_pin = 48;
const int echo_pin = 46;

// Distance measurement variables
float hcTiming = 0.0;
float hcDist = 0.0;

// ------------------------------ ADXL335 ------------------------------ //

// Pin assignments
const int xPin = A5;
const int yPin = A6;
const int zPin = A7;

// ------------------------------ STRAIN ------------------------------ //

// Pin assignment
const int strainPin = A0; // Analog pin for module output

// Reading variables
int rawValue = 0; // Variable to store raw reading
int baseStrain = 0; // Base measurement for strain

// ------------------------------ WIND ------------------------------ //

// Pin assignments
const int pinRV  = A8;  // Wind analog output
const int pinTMP = A9;  // Temperature analog output

// Reading/measurement variables
int wind = 0; // Analog measurement from pinRV
int temp = 0; // Analog measurement from pinTMP
int baseWind = 0; // Baseline wind measurement
int baseTemp = 0; // Baseline temperature measurement

// Indicates which test is being performed on the wind sensor
enum WindTest {TEMP,BLOW};
WindTest windTest = TEMP;

// ------------------------------ LOAD ------------------------------ //

// Pin assignments
const int LOADCELL_DOUT_PIN = 45;
const int LOADCELL_SCK_PIN = 47;

// Measurement variables
int32_t load;
int32_t maxLoad;
int32_t baseLoad;

// Scale the amplifier output
Adafruit_HX711 scale(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);

// ============================================================================================================ SETUP ============================================================================================================ //
void setup() {
  // Begin serial communication
  Serial.begin(115200);
  while (!Serial) delay(1000); // Wait until serial monitor is opened
  delay(3000); // 3-second delay to give the user some time to prepare

  while (!all_init) {
    Serial.print("Would you like to test the "); 
    printSensor();
    Serial.println(" in this test run? (y/n): ");
    while ((c != 'y') && (c != 'n')) {
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
          case HALL:
            hallInitialize();
            break;
          case HCSR04:
            hcInitialize();
            break;
          case ADXL335:
            adxlInitialize();
            break;
          case STRAIN:
            strainInitialize();
            break;
          case WIND:
            windInitialize();
            break;
          case LOAD:
            loadInitialize();
            break;
        }
        if (is_working[sensor]) {
          printSensor(); 
          Serial.println(" initialized.");
        }
        sensor = static_cast<Select>(sensor + 1);
      } else if (c == 'n') {
        sensor = static_cast<Select>(sensor + 1);
      }
    }
    c = '\0';
    if (sensor > LOAD) {
      all_init = true; // This will end the while loop
    }
  }

  nextSensor(); // Set the sensor to be tested

  // Prompt user input to begin testing
  Serial.println("Press SPACE + ENTER to begin testing.");
}

// ============================================================================================================ FSM ============================================================================================================ //
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
              case ADS1115: adsPrompt();    break;
              case LIS3DH:  accelPrompt();  break;
              case VL53L1X: vlxPrompt();    break;
              case PEC11:   pecPrompt();    break;
              case HALL:    hallPrompt();   break;
              case HCSR04:  hcPrompt();     break;
              case ADXL335: accelPrompt();  break;
              case STRAIN:  strainPrompt(); break;
              case WIND:    windPrompt();   break;
              case LOAD:    loadPrompt();   break;
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
              updateMax();

              if (c == ' ') {
                lisDecide();
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
              if (c == ' ') {
                pecDecide(); // Do not need to expressly read the sensor (done through interrupts), just decide here if it works
              }
              break;
            case HALL:
              s_curr = digitalRead(hallPin); // read sensor state
              //Serial.println(s_curr); //for troubleshooting only

              if (c == ' ') {
                hallDecide();
              }
              break;
            case HCSR04:
              hcRead();

              if (c == ' ') {
                hcDecide();
              }
              break;
            case ADXL335:
              adxlRead(); // Read from the ADXL335
              updateMax(); // Update max values for x, y, and z measurements
              adxlDecide(); // Only runs if spacebar is pressed
              break;
            case STRAIN:
              rawValue = analogRead(strainPin);
              // Serial.print("Raw value: "); Serial.println(rawValue); keep for debugging

              if (c == ' ') {
                strainDecide();
              }
              break;
            case WIND:
              windRead();

              if (c == ' ') {
                windDecide();
              }
              break;
            case LOAD:
              loadRead();

              if (c == ' ') {
                loadDecide();
              }
          }
          break;
        case VERDICT:
          // Print whether or not the sensor is working
          if (!printedSucc) {
            giveVerdict(); // Let the user know whether or not the sensor is working

            nextSensor(); // Move onto the next sensor (and/or figure out if all have been tested)

            // If not all of the sensors have been tested, ask the user if they would like to continue on to the next sensor
            if (!all_tested) {
              Serial.println("Would you like to test another sensor? (y/n): ");
              printedFail = true; // set to true so that we execute properly through the FSM
              printedSucc = true; // see above
            }
          }
          break;
      }
    } else if (c == 'y') {
      reset4next(); // Reset important loop logic for next sensor test
      
      Serial.println("Press SPACE + ENTER to begin testing.");
    } else if (c == 'n') {
      all_tested = true; // Will print goodbye message on the next loop iteration
    }
  } else { // If all of the sensors have been tested
    Serial.println("All tests completed. Push reset button to run another round of tests.");
    printSummary();
    while(1); // Hold indefinitely while the Mega is powered
  }
}

// ============================================================================================================ FUNCTIONS ============================================================================================================ //

// ------------------------------ GENERAL ------------------------------ //

// Determine what key a user pressed
char charPressed() {
  if(Serial.available()) {
    char c = Serial.read();
    delay(1); // wait a millisecond to ensure that the buffer is read before being emptied
    while (Serial.available()) {
      Serial.read(); // clear buffer
    }
    return c;
  }
  return '\0';
}

void printSensor() {
  switch (sensor) {
    case ADS1115: Serial.print("ADS1115"); break;
    case LIS3DH: Serial.print("LIS3DH"); break;
    case VL53L1X: Serial.print("VL53L1X"); break;
    case BREAKOUT: Serial.print("SD Card Breakout Board +"); break;
    case PEC11: Serial.print("PEC11"); break;
    case HALL: Serial.print("Hall Effect Module"); break;
    case HCSR04: Serial.print("HC-SR04"); break;
    case ADXL335: Serial.print("ADXL335"); break;
    case STRAIN: Serial.print("Runeskee Strain Gauge"); break;
    case WIND: Serial.print("Modern Device Wind Sensor Rev C"); break;
    case LOAD: Serial.print("HX711 Load Cell Amplifier"); break;
  }
}

// Determine which sensor is next from the list of sensors to test
void nextSensor() {
  all_tested = true;
  // Iterate through the to_test array until we find a sensor that will be tested
  for (int ii = 0; ii < 11; ii++) {
    if (to_test[ii] == true) {
      sensor = static_cast<Select>(ii); // ii should correspond to the index of the sensor enum (ex. 0 corresponds to ADS1115)
      to_test[ii] = false; // eliminates repeated tests of the same sensor
      all_tested = false; // if we find that there's a sensor we haven't tested, set this false so that we actually test it
      break;
    }
  }
}

// Give a verdict on sensor functionality
void giveVerdict() {
  if (!printedSucc) {
    printSensor();
    if (is_working[sensor]) {
      Serial.println(" is working.");
      printedSucc = true;
    } else {
      Serial.println(" is not working.");
      printedFail = true;
    }
  }
  tested[sensor] = true;
}

// Function that resets important loop logic for next sensor
void reset4next() {
  //is_working = true;
  printedFail = false;
  printedSucc = false;
  status = WAITING;
  level = POS;
  direction = X;
  proximity = NEAR;
  rotDir = CW;
  for (int ii = 0; ii < 3; ii++) {
    vals[ii] = 0;
    maxVals[ii] = 0;
  }
}

void printSummary() {
  Serial.println("Summary of test run: ");
  for (int ii = 0; ii < 11; ii++) {
    sensor = static_cast<Select>(ii);
    Serial.print("\t"); Serial.print(sensor + 1); Serial.print(". ");
    printSensor();
    if (tested[sensor] == true) {
      if (is_working[sensor] == true) {
        Serial.println(" is working.");
      } else {
        Serial.println(" is not working.");
      }
    } else {
      Serial.println(" was not tested in this run.");
    }
  }
}

// ------------------------------ ADS1115 ------------------------------ //

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
    Serial.println("Beginning ADS1115 test. Turn bench power supply on and set to 5.1 V. Then connect the positive lead to the A0 pin.");
    Serial.println("Press SPACE + ENTER to conclude test.");
  } else {
    Serial.println("Beginning second ADS1115 test. First, turn off the power supply.");
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
        is_working[ADS1115] = false;
        status = VERDICT;
    } else {
      Serial.println("Test complete. Press SPACE + ENTER to begin next test.");
      status = WAITING;
    }
  } else {
    status = VERDICT;
    Serial.println("Test complete.");
    if (voltage > negMax) {
      is_working[ADS1115] = false;
    }
  }
}

// ------------------------------ LIS3DH ------------------------------ //

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

// Give user instructions for testing LIS3DH and ADXL335
void accelPrompt() {
  switch (direction) {
  // Test x,y,z directions in that order to ensure all are working.
  case X: // x-direction
    Serial.println("Move shield rapidly forward and backward. Press SPACE + ENTER to conclude test.");
    break;
  case Y: // y-direction
    Serial.println("Move shield rapidly from left to right. Press SPACE + ENTER to conclude test.");
    break;
  case Z: // z-direction
    Serial.println("Move board rapidly upward and downward. Press SPACE + ENTER to conclude test.");
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
void updateMax() {
  if (vals[direction] > maxVals[direction]) {
    maxVals[direction] = vals[direction];
  }
}

// Decide whether LIS3DH is working
void lisDecide() {
  if (direction == Z) {
    status = VERDICT;
    Serial.println("Testing complete.");
    lisVerdict();
  } else {
    status = READING;
    lisVerdict();
    direction = (Axis)(direction + 1);
  }
}

// Decide whether LIS3DH is working
void lisVerdict() {
  if (direction == Z) {
    if (abs(maxVals[Z]) < 11.0) {
      is_working[LIS3DH] = false;
      status = VERDICT;
    }
  } else {
    if (abs(maxVals[direction]) < 2.0) {
      is_working[LIS3DH] = false;
      status = VERDICT;
    }
  }
}

// ------------------------------ VL53L1X ------------------------------ //

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
    Serial.println("Beginning VL53L1X test. Place hand 6 inches above VL53L1X, then press SPACE + ENTER to conclude test.");
  } else {
    Serial.println("Beginning second VL53L1X test. Remove hand from above VL53L1X, then press SPACE + ENTER to conclude test.");
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
      is_working[VL53L1X] = false;
      status = VERDICT;
    } else {
      Serial.println("Test complete. Press SPACE + ENTER to begin next test.");
      status = WAITING;
    }
  } else {
    status = VERDICT;
    Serial.println("Test complete.");
    if (distance < 1000) {
      is_working[VL53L1X] = false;
    }
  }
}

// ------------------------------ SD Card Breakout Board + ------------------------------ //

// Initialize SD Card Breakout Board
void sdInitialize() {
  Serial.print("Initializing SD card... ");
  pinMode(chipSelect, OUTPUT); // Set the CS pin (Arduino 10) as an output from the Mega

  // Alert user if sensor fails to connect
  if (!SD.begin(chipSelect)) {
    Serial.println("initialization failed!");
    is_working[BREAKOUT] = false;
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
  while (c != 'y' && c != 'n') {
    c = charPressed();
  }

  if (c == 'n') {
    is_working[BREAKOUT] = false;
  }
  status = VERDICT;
}

// ------------------------------ PEC11 ------------------------------ //

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
    Serial.println("Beginning PEC11 test. Slowly rotate the encoder a few clicks clockwise, then press SPACE + ENTER to conclude test.");
  } else {
    Serial.println("Beginning PEC11 test. Slowly rotate the encoder a few clicks counterclockwise, then press SPACE + ENTER to conclude test.");
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
  if (c == ' ') {
    if (rotDir == CW) {
      status = WAITING;
      rotDir = CCW;
      if (!Encoded || EncoderValue <= 0) {
        is_working[PEC11] = false;
        status = VERDICT;
      } else {
        Serial.println("Test complete. Press SPACE + ENTER to begin next test.");
      }
    } else {
      status = VERDICT;
      Serial.println("Test complete.");
      if (!Encoded || EncoderValue >= 0) {
        is_working[PEC11] = false;
      }
      detachInterrupt(0);
      detachInterrupt(1);
    }
  }
}

// ------------------------------ HALL ------------------------------ //

// Initialize hall effect sensor
void hallInitialize() {
  // Set pin modes
  pinMode(hallPin,INPUT);

  // Read initial sensor value to screen for false positives
  Serial.println("Hold magnet away from SY-M213.");
  delay(2000);
  s_curr = digitalRead(hallPin);
  if (s_curr == 1) {
    is_working[HALL] = false;
    Serial.println("SY-M213 gave false positive and is not working.");
    to_test[5] = false; // Indicate that this sensor has already been tested and is not working
  }
}

// Prompt user to begin SY-M213 test
void hallPrompt() {
  Serial.println("Place and hold a magnet next to the SY-M213 next to the BJT on the back side of the board, then press SPACE + ENTER to conclude test.");
}

// Decide whether SY-M213 is working
void hallDecide() {
  is_working[HALL] = (s_curr == 1);
  status = VERDICT;
}

// ------------------------------ HC-SR04 ------------------------------ //

// Initialize HC-SR04
void hcInitialize() {
  // Initialize pins
  pinMode(trig_pin, OUTPUT); digitalWrite(trig_pin, LOW);
  pinMode(echo_pin, INPUT);
}

// Prompt user to begin HC-SR04 test
void hcPrompt() {
  if (proximity == NEAR) {
    Serial.println("Beginning HC-SR04 test. Place hand 6 inches in front of HC-SR04, then press SPACE + ENTER to conclude test.");
  } else {
    Serial.println("Beginning second HC-SR04 test. Remove hand, then press SPACE + ENTER to conclude test.");
  }
}

// Read from the HC-SR04
void hcRead() {
  // new measurement for the taking!
  digitalWrite(trig_pin, LOW);
  delay(2);

  digitalWrite(trig_pin, HIGH);
  delay(10);
  digitalWrite(trig_pin, LOW);

  hcTiming = pulseIn(echo_pin, HIGH);

  hcDist = ((hcTiming * 0.034) / 2)/(2.54);
  if (hcDist == 0) {
    // something went wrong!
    Serial.print(F("Couldn't get distance"));
    return;
  }
}

// Decide whether HC-SR04 is working
void hcDecide() {
  if (proximity == NEAR) {
    proximity = FAR;
    if (hcDist >= 8 || hcDist <= 3) {
      is_working[HCSR04] = false;
      status = VERDICT;
    } else {
      Serial.println("Test complete. Press SPACE + ENTER to begin next test.");
      status = WAITING;
    }
  } else {
    status = VERDICT;
    Serial.println("Test complete.");
    if (hcDist < 12) {
      is_working[HCSR04] = false;
    }
  }
}

// ------------------------------ ADXL335 ------------------------------ //

// Initialize ADXL335
void adxlInitialize() {
  Serial.println("Measuring initial accelerations. Keep board on a flat surface...");
  delay(3000); // Give the user a chance to set the board down
  for (int ii = 0; ii < 400; ii++) {
    // Get a new sensor reading with normalized accelerations
    adxlRead();
    
    // Decide whether sensor is working based on offsets. x and y should be close to 0; z should be close to 9.81.
    if (fabs(vals[0]) > 1.0 || fabs(vals[1]) > 1.0 || fabs(vals[2]) > 11.0 || fabs(vals[2]) < 7.0) {
      Serial.println("Sensor measurements do not match expected values.");
      is_working[ADXL335] = false;
      status = VERDICT;
      break;
    }
    // Short delay
    delay(1);
  }
}

// Read from ADXL335
void adxlRead() {
  // Read raw analog values
  int rawX = analogRead(xPin);
  int rawY = analogRead(yPin);
  int rawZ = analogRead(zPin);

  // Convert to voltage
  float xVolt = map(rawX, 0,1023,0,5000);
  float yVolt = map(rawY, 0,1023,0,5000);
  float zVolt = map(rawZ, 0,1023,0,5000);

  // Convert voltage to acceleration in m/s^2
  vals[0] = 9.81*(map(xVolt, 0, 3300, -5000, 5000)/1000.0);
  vals[1] = 9.81*(map(yVolt, 0, 3300, -5000, 5000)/1000.0);
  vals[2] = 9.81*(map(zVolt, 0, 3300, -5000, 5000)/1000.0);
}

// Decide whether ADXL335 is working
void adxlDecide() {
  if (direction == Z) {
    if (c == ' ') {
      //Serial.println(fabs(maxVals[direction]));
      status = VERDICT;
      Serial.println("Testing complete.");
      adxlVerdict();
    }
  } else {
    if (c == ' ') {
      //Serial.println(fabs(maxVals[direction]));
      status = READING;
      adxlVerdict();
      direction = (Axis)(direction + 1);
    }
  }
}

// Decide whether ADXL335 is working
void adxlVerdict() {
  if (direction == Z) {
    if (fabs(maxVals[Z]) < 2.0) {
      is_working[ADXL335] = false;
    }
  } else {
    if (fabs(maxVals[direction]) < 2.0) {
      is_working[ADXL335] = false;
      status = VERDICT;
    }
  }
}

// ------------------------------ STRAIN ------------------------------ //

// Provide user instruction for strain gauge test
void strainPrompt() {
  Serial.println("Beginning test. Lightly bend the strain gauge toward the side with the solder beads, then press SPACE + ENTER to conclude test.");
}

// Initialize strain gauge
void strainInitialize() {
  // Get baseline strain data
  Serial.print("Do not touch strain gauge. Initializing...");
  delay(1000);
  baseStrain = analogRead(strainPin);
  Serial.println(" done.");
}

// Decide whether the strain gauge is working
void strainDecide() {
  status = VERDICT;
  if (abs(rawValue - baseStrain) < 5) {
    is_working[STRAIN] = false;
  }
}

// ------------------------------ WIND ------------------------------ //

// Initialize wind sensor
void windInitialize() {
  // Set pin modes
  pinMode(pinRV,INPUT);
  pinMode(pinTMP,INPUT);

  // Get baseline values for wind and temp
  baseWind = analogRead(pinRV);
  baseTemp = analogRead(pinTMP);
}

// Provide user instruction for wind sensor test
void windPrompt() {
  if (windTest == TEMP) {
    Serial.println("Beginning wind sensor test. Hold the fins at the end of the sensor with thumb and forefinger, then press SPACE + ENTER.");
  } else {
    Serial.println("Beginning second wind sensor test. Blow on the fins at the end of the sensor, then press SPACE + ENTER.");
  }
}

// Read from the wind sensor
void windRead() {
  if (windTest == TEMP) {
    temp = analogRead(pinTMP);
  } else {
    wind = analogRead(pinRV);
  }
}

// Decide whether the wind sensor is working
void windDecide() {
  if (windTest == TEMP) {
    windTest = BLOW;
    status = WAITING;
    if (fabs(temp - baseTemp) < 10) {
      is_working[WIND] = false;
      status = VERDICT;
    } else {
      Serial.println("Test complete. Press SPACE + ENTER to begin next test.");
    }
  } else {
    status = VERDICT;
    if (fabs(wind - baseWind) < 10) {
      is_working[WIND] = false;
    }
  }
}

// ------------------------------ LOAD ------------------------------ //

// Initialize load sensor
void loadInitialize() {
  scale.begin(); // Initialize amplifier connection

  // Get baseline loading data
  Serial.print("Do not touch load cell. Initializing...");
  delay(1000);
  baseLoad = scale.readChannelBlocking(CHAN_A_GAIN_128);
  Serial.println(" done.");
}

// Provide user instruction for load cell test
void loadPrompt() {
  Serial.print("Hold load cell with thumbs on the same side as the screw tops, then bend the load cell. ");
  Serial.println("Press SPACE + ENTER to conclude test.");
}

// Read from the HX711
void loadRead() {
  load = scale.readChannelBlocking(CHAN_A_GAIN_128);
  if ((load > maxLoad) && (load != 0)) {
    maxLoad = load;
  }
  delay(1);
}

// Decide whether the HX711 is working
void loadDecide() {
  status = VERDICT;
  Serial.println(maxLoad);
  Serial.println(baseLoad);
  if (fabs(maxLoad) - fabs(baseLoad) < 5000) {
    is_working[LOAD] = false;
  }
}