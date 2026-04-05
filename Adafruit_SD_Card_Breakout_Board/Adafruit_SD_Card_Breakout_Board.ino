// Written by Benjamin Wenger on 4-3-26
// Last revision 4-3-26
// Functionality test code for Adafruit SD Card Breakout Board+

/*
This software test assumes that the card is in FAT32 format. 
Will write something to the card, then read it from the card and ensure that the card can store data properly.
*/

// Include necessary libraries
#include <SPI.h>
#include <SD.h>

// Declare assignment for chip select (CS) pin
const int chipSelect = 10;

// Declare variable to hold file content from SD card
std::string contents = "Hello from SD Card!";

// State of loop operations
enum State {WAITING, TESTING, VERDICT};
State status = WAITING;

// Stage of sensor testing
enum SdStage {WRITING,READING};
SdStage sdStage = WRITING;

// Variables to indicate if sensor works (default is yes)
bool is_working = true;
bool printedFail = false;
bool printedSucc = false;

void setup() {
  // Begin serial communication
  Serial.begin(115200);
  while (!Serial) delay(1000); // Wait until serial monitor is opened
  delay(3000); // 3-second delay to give the user some time to prepare

  // Initialize SD card
  Serial.print("Initializing SD card... ");
  pinMode(chipSelect, OUTPUT);

  // Alert user if sensor fails to connect
  if (!SD.begin(chipSelect)) {
    Serial.println("initialization failed!");
    return;
  }

  Serial.println("done.");

  // Prompt user to begin the test; wait for input before proceeding
  Serial.println("Press SPACE + ENTER to begin test.");
  while (!spacePressed()) {}

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

void loop() {
  // Nothing runs here--all code runs once in setup
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