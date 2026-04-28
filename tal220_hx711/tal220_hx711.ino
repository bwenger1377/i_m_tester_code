// Written by Benjamin Wenger on 4-28-26
// Last revision 4-28-26
// Functionality test code for TAL220 load cell / HX711 amplifier combination

// Include necessary libraries
#include <Adafruit_HX711.h>

// Pin assignments
const int LOADCELL_DOUT_PIN = 45;
const int LOADCELL_SCK_PIN = 47;

// Measurement variables
int32_t load;
int32_t maxLoad;
int32_t baseLoad;

// Scale the amplifier output
Adafruit_HX711 scale(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);

// State of loop operations
enum State {WAITING,READING,VERDICT};
State status = WAITING;

// Indicate whether a sensor works / if a verdict has been passed
bool printedFail = false;
bool printedSucc = false;
bool is_working = true;


void setup() {
  // Begin serial communication
  Serial.begin(115200);
  while (!Serial) delay(1000); // Wait until serial monitor is opened
  delay(3000); // 3-second delay to give the user some time to prepare

  loadInitialize();

  // Prompt user input to begin testing
  Serial.println("Press SPACE + ENTER to begin testing.");
}

void loop() {
  if (!printedFail) {
    switch (status) {
      case WAITING:
        if (spacePressed()) {
          status = READING;
          loadPrompt();
        }
        break;
      case READING:
        loadRead();
        if (spacePressed()) {
          loadDecide();
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
  if (load > maxLoad) {
    maxLoad = load;
  }
  delay(1);
}

// Decide whether the HX711 is working
void loadDecide() {
  status = VERDICT;
  if (fabs(maxLoad - baseLoad) < 1000) {
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