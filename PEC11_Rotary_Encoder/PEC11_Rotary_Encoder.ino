// Written by Benjamin Wenger on 3-31-26
// Last updated 4-20-26
// Functionality Test Code for PEC11 Rotary Encoder

// State of loop operations
enum State {WAITING, READING, VERDICT};
State status = WAITING;

// Direction of rotation
enum Direction {CW,CCW};
Direction rotDir = CW;

// Variables to indicate if sensor works (default is yes)
bool is_working = true;
bool printedFail = false;
bool printedSucc = false;

// Declaring pin locations
const int enc_bottom_pin = 2;         //Encoder pin that is not the middle
const int enc_top_pin = 3;            //Encoder pin that is not the middle (the other one)

// Variables needed to read rotational encoder values
int Encoded = 0;                    //Current value of general rotation
long LastEncoderValue = 0;          //Last value of the encoder
int bin_sum = 0;                    //Sum of binary sequence
volatile int LastEncoded = 0;       //Last value of the general rotation
volatile long EncoderValue = 0;     //Current value of the encoder
int enc_bottom_val;                 //Reading from enc_bottom_pin
int enc_top_val;                    //Reading from enc_top_pin

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  while (!Serial) delay(1000); // Wait until serial monitor is opened
  delay(3000); // 3-second delay to give the user some time to prepare

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

  Serial.println("Press SPACE + ENTER to begin testing.");
}
 
void loop() {
  // Only execute code in the loop if it is not known whether or not the sensor works
  if (!printedFail) {
    switch (status) {
      case WAITING: // waiting for user input
        if (spacePressed()) {
          status = READING; // will execute code in second case next time loop repeats
          if (rotDir == CW) { // prompt the user based on the direction of encoder rotation
            Serial.println("Beginning test. Slowly rotate the encoder a few clicks clockwise, then press SPACE + ENTER to conclude test.");
          } else {
            Serial.println("Beginning test. Slowly rotate the encoder a few clicks counterclockwise, then press SPACE + ENTER to conclude test.");
          }
        }
        break;
      case READING:
        if (rotDir == CW) {
          if (spacePressed()) {
            status = WAITING;
            rotDir = CCW;
            Serial.println("Test complete. Press SPACE + ENTER to begin next test.");
            if (!Encoded || EncoderValue <= 0) {
              is_working = false;
              status = VERDICT;
            }
          }
        } else {
          if (spacePressed()) {
            status = VERDICT;
            Serial.println("Test complete.");
            if (!Encoded || EncoderValue >= 0) {
              is_working = false;
            }
          }
        }
        break;
      case VERDICT:
        if (!printedSucc) {
          Serial.println(EncoderValue);
          if (is_working) {
            Serial.println("PEC11 Rotary Encoder is working.");
            printedSucc = true;
          } else {
            Serial.println("PEC11 Rotary Encoder is not working.");
            printedFail = true;
          }
        }
        break;
    }
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