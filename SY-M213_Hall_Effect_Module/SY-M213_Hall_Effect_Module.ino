// Written by Benjamin Wenger on 2-3-26
// Last Revision 4-28-26
// Functionality Test Code for SY-M213 Hall Effect Module

//Confirm that the presence of a magnet changes the digital pin reading on the Arduino. 
//If the reading does not change after prompting the user to put a magnet near the BJT at the top, then the sensor is defective.

// Declare pin number
const int hallPin = 49;   // substitute for digital pin connected to SY-M213 output

// Sensor reading variables
int s_curr = 0; // Current sensor reading
int s_prev = 0; // Previous sensor reading

// For loop logic
int status = 1; // 1 - waiting for user, 2 - reading sensor value, 3 - deciding whether the sensor works

// Variables to indicate if sensor works (default is yes)
bool is_working = 1;
bool printedFail = 0;
bool printedSucc = 0;

void setup() {
  // Set pin modes
  pinMode(hallPin, INPUT);

  // Initialize baudrate and serial monitor
  Serial.begin(115200);
  while (!Serial) delay(1000); // Wait until serial monitor is opened
  delay(3000); // 5-second delay to give the user some time to prepare

  // Read initial sensor value to screen for false positives
  Serial.println("Hold magnet away from sensor.");
  delay(2000);
  s_curr = digitalRead(hallPin);
  if (s_curr == 1) {
    is_working = 0;
    Serial.println("Sensor gave false positive and is not working.");
    printedFail = 1;
  } else {
    Serial.println("Sensor is working so far.");
    Serial.println("Place a magnet next to the BJT, then press SPACE + ENTER to begin test.");
  }
}

void loop() {
  if (!printedFail) {
    switch (status) {
      case 1: // Waiting
        if (Serial.available()) {
          char message = Serial.read();
          if (message == ' ') {
            status = 2;
            Serial.println("Testing. Do not remove magnet.");
            Serial.println("Press SPACE + ENTER again to end the test.");
          }
        }
        break;
      case 2: // Reading and logging sensor data
        s_curr = digitalRead(hallPin); // read sensor state
        //Serial.println(s_curr); //for troubleshooting only
        if (s_curr != 1 & s_prev != 1) {
          is_working = 0;
        } else {
          is_working = 1;
        }
        if (Serial.available()) {
          char message = Serial.read();
          if (message == ' ') {
            status = 3;
            Serial.println("Remove magnet.");
          }
        }
        break;
      case 3: // Verdict
        // Code for this section
        if (!printedSucc) {
          if (is_working) {
            Serial.println("Sensor is working.");
            printedSucc = 1;
          } else {
            Serial.println("Sensor is not working.");
            printedFail = 1;
          }
        }
        break;
    }
  }
}