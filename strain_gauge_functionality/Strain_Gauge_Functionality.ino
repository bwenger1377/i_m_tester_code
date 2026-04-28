// Written by Benjamin Wenger on 4-2-26
// Last revision 4-27-26
// Runeskee Strain Gauge - Functionality Test

// Pin assignment
const int strainPin = A0; // Analog pin for module output

// Reading variables
int rawValue = 0; // Variable to store raw reading
int baseStrain = 0; // Base measurement for strain

// State of loop operations
enum State {WAITING, READING, VERDICT};
State status = WAITING;

// Variables to indicate if sensor works (default is yes)
bool is_working = true;
bool printedFail = false;
bool printedSucc = false;

void setup() {
  // Begin serial communication
  Serial.begin(115200);
  while (!Serial) delay(1000); // Wait until serial monitor is opened
  delay(3000); // 3-second delay to give the user some time to prepare

  // Initialize strain gauge
  strainInitialize();

  // Initialize Test  
  Serial.println("Press SPACE + ENTER to begin testing.");
}

void loop() {
  // Only execute code in the loop if it is not known whether or not the sensor works
  if (!printedFail) {
    switch (status) {
      case WAITING: // waiting for user input
        if (spacePressed()) {
          status = READING; // will execute code in second case next time loop repeats
          strainPrompt();
        }
        break;
      case READING:
        rawValue = analogRead(strainPin);
        // Serial.print("Raw value: "); Serial.println(rawValue); keep for debugging

        if (spacePressed()) {
          strainDecide();
        }
        break;
      case VERDICT:
        if (!printedSucc) {
          giveVerdict();
        }
        break;
    }
  }
}

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
    is_working = false;
  }
}

// Give a verdict on sensor functionality
void giveVerdict() {
  if (!printedSucc) {
    if (is_working) {
      Serial.println("Strain gauge is working.");
      printedSucc = true;
    } else {
      Serial.println("Strain gauge is not working.");
      printedFail = true;
    }
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