// Written by Benjamin Wenger on 4-27-26
// Last Revision 4-27-26
// Modern Device Wind Sensor Rev C Functionality Test

// State of loop operations
enum State {WAITING,READING,VERDICT};
State status = WAITING;

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

// Loop logic variables
bool printedFail = false;
bool printedSucc = false;
bool is_working = true;

// --- Sensor Reading Functions ---
float readVoltage(int pin) {
  int raw = analogRead(pin);
  return (raw * 5.0) / 1023.0;
}

float readWind() {
  return readVoltage(pinRV);
}

float readTemp() {
  return readVoltage(pinTMP);
}

void setup() {
  // Begin serial communication
  Serial.begin(115200);
  while (!Serial) delay(1000); // Wait until serial monitor is opened
  delay(3000); // 3-second delay to give the user some time to prepare

  // Initialize wind sensor
  windInitialize();

  // Prompt user input to begin testing
  Serial.println("Press SPACE + ENTER to begin testing.");
}

void loop() {
  if (!printedFail) {
    switch (status) {
      case WAITING:
        if (spacePressed()) {
          status = READING;
           
          windPrompt();
        }
        break;
      case READING: 
        windRead();

        if (spacePressed()) {
          windDecide();
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
      is_working = false;
      status = VERDICT;
    } else {
      Serial.println("Test complete. Press SPACE + ENTER to begin next test.");
    }
  } else {
    status = VERDICT;
    if (fabs(wind - baseWind) < 10) {
      is_working = false;
    }
  }
}

// Give a verdict on sensor functionality
void giveVerdict() {
  if (!printedSucc) {
    if (is_working) {
      Serial.println("Wind sensor is working.");
      printedSucc = true;
    } else {
      Serial.println("Wind sensor is not working.");
      printedFail = true;
    }
  }
}