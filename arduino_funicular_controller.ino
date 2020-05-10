#include <NewPing.h>
//#include <hcsr04.h>
#include <SPI.h>
#include <deprecated.h>
#include <MFRC522.h>
//#include <MFRC522Extended.h>
#include <require_cpp11.h>


/*
   --------------------------------------------------------------------------------------------------------------------
   Example sketch/program showing how to read data from a PICC to serial.
   --------------------------------------------------------------------------------------------------------------------
   This is a MFRC522 library example; for further details and other examples see: https://github.com/miguelbalboa/rfid

   Example sketch/program showing how to read data from a PICC (that is: a RFID Tag or Card) using a MFRC522 based RFID
   Reader on the Arduino SPI interface.

   When the Arduino and the MFRC522 module are connected (see the pin layout below), load this sketch into Arduino IDE
   then verify/compile and upload it. To see the output: use Tools, Serial Monitor of the IDE (hit Ctrl+Shft+M). When
   you present a PICC (that is: a RFID Tag or Card) at reading distance of the MFRC522 Reader/PCD, the serial output
   will show the ID/UID, type and any data blocks it can read. Note: you may see "Timeout in communication" messages
   when removing the PICC from reading distance too early.

   If your reader supports it, this sketch/program will read all the PICCs presented (that is: multiple tag reading).
   So if you stack two or more PICCs on top of each other and present them to the reader, it will first output all
   details of the first and then the next PICC. Note that this may take some time as all data blocks are dumped, so
   keep the PICCs at reading distance until complete.

   @license Released into the public domain.

   Typical pin layout used:
   -----------------------------------------------------------------------------------------
               MFRC522      Arduino       Arduino   Arduino    Arduino          Arduino
               Reader/PCD   Uno/101       Mega      Nano v3    Leonardo/Micro   Pro Micro
   Signal      Pin          Pin           Pin       Pin        Pin              Pin
   -----------------------------------------------------------------------------------------
   RST/Reset   RST          9             5         D9         RESET/ICSP-5     RST
   SPI SS      SDA(SS)      10            53        D10        10               10
   SPI MOSI    MOSI         11 / ICSP-4   51        D11        ICSP-4           16
   SPI MISO    MISO         12 / ICSP-1   50        D12        ICSP-1           14
   SPI SCK     SCK          13 / ICSP-3   52        D13        ICSP-3           15
*/


#define SS_PIN                  10         // Configurable, see typical pin layout above
#define RST_PIN                  9         // Configurable, see typical pin layout above
#define ECHO_PIN                 8         // ultrasonic echo
#define TRIG_PIN                 7         // ultrasonic trig
#define STARTSTOP_BUTTON_PIN     6         // button pin
#define MOTOR_EN_PIN             5         // set PWM speed 0-255
#define MOTOR_DIRA_PIN           4         // left, if HIGH and right is LOW,
#define MOTOR_DIRB_PIN           3         // right if HIGH and left is LOW


MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance
//HCSR04 hcsr04(TRIG_PIN, ECHO_PIN, 20, 4000); // ultrasonic sensor instance
NewPing sonar(TRIG_PIN, ECHO_PIN, 200);

void setup() {
  Serial.begin(9600);   // Initialize serial communications with the PC
  // start-stop button
  pinMode(STARTSTOP_BUTTON_PIN, INPUT_PULLUP);
  // init motor
  //  pinMode(MOTOR_EN_PIN, OUTPUT);
  //  pinMode(MOTOR_DIRA_PIN, OUTPUT);
  //  pinMode(MOTOR_DIRB_PIN, OUTPUT);
  //  digitalWrite(MOTOR_EN_PIN, LOW);
  //  digitalWrite(MOTOR_DIRA_PIN, LOW);
  //  digitalWrite(MOTOR_DIRB_PIN, LOW);

  //? while (!Serial);    // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
  SPI.begin();      // Init SPI bus
  mfrc522.PCD_Init();   // Init MFRC522
  mfrc522.PCD_SetAntennaGain(MFRC522::PCD_RxGain::RxGain_max);
  mfrc522.PCD_AntennaOn();

  delay(4);       // Optional delay. Some board do need more time after init to be ready, see Readme
  mfrc522.PCD_DumpVersionToSerial();  // Show details of PCD - MFRC522 Card Reader details
  Serial.println(F("Scan PICC to see UID, SAK, type, and data blocks..."));
}

int distance_cm() {
  int distance = sonar.ping_cm();
  if (distance == 0) {
    distance = 9999;
  }
  return distance;
}
//long distanceMillsAvg(NewPing hcsr04, int waitMillis, int count) {
//  long min, max, avg, d;
//  min = 9999;
//  max = 0;
//  avg = d = 0;
//
//  if (waitMillis < 1) {
//    waitMillis = 1;
//  }
//
//  if (count < 1) {
//    count = 1;
//  }
//
//  for (int x = 0; x < count + 2; x++) {
//    d = hcsr04.distanceInMillimeters() / 2;
//
//    if (d < min) {
//      min = d;
//    }
//
//    if (d > max) {
//      max = d;
//    }
//
//    avg += d;
//    delay(waitMillis);
//  }
//
//  // substract highest and lowest value
//  avg -= (max + min);
//  // calculate average
//  avg /= count;
//  return avg * 2;
//}

const byte car_uid[][7] = {
  {0x04, 0xC1, 0xF4, 0x72, 0x84, 0x5C, 0x80},
  {0x04, 0x4F, 0xF5, 0x72, 0x84, 0x5C, 0x81}
};

int array_len(byte arr[]) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsizeof-array-argument"
  return sizeof(*arr) / sizeof(byte);
#pragma GCC diagnostic pop
}

template <typename T> int array_len(const T arr[]) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsizeof-array-argument"
  return sizeof(arr) / sizeof(T);
#pragma GCC diagnostic pop
}

void dump_uid(int len, const byte uid[]) {
  for (byte i = 0; i < len; i++) {
    Serial.print(uid[i] < 0x10 ? " 0" : " ");
    Serial.print(uid[i], HEX);
  }
}
bool is_uid(int len, const byte uid_left[], const byte uid_right[]) {
  //  if (array_len(uid_left) != array_len(uid_right)) {
  //    return false;
  //  }
  //  int len = array_len(uid_left);
  for (byte i = 0; i < len; i++) {
    if (uid_left[i] != uid_right[i]) {
      return false;
    }
  }
  return true;
}

int check_car_in_station(byte uidByte[]) {
  const int cars[] = { -1, 1 };
  for (int i = 0; i < 2; i++) {
    if (is_uid(7, mfrc522.uid.uidByte, car_uid[i])) {
      return cars[i];
    }
  }
  return 0;
}

// -1: no car detected, else 0-based car number (0,1)
int determine_car_in_station() {
  int car_in_station = 0;
  mfrc522.PCD_AntennaOn();
  if ( mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    Serial.print("detected car");
    dump_uid(7, mfrc522.uid.uidByte);
    Serial.println();
    car_in_station = check_car_in_station(mfrc522.uid.uidByte);
  }
  mfrc522.PCD_AntennaOff();
  return car_in_station;
}

bool startstop_button_up_old = false;
bool startstop_button_pressed() {
  const int MAX_LOOP = 5;
  // debounce
  int startstop_state = 0;
  for (int i = 0; i < MAX_LOOP; i++) {
    startstop_state += digitalRead(STARTSTOP_BUTTON_PIN);
    delay(1);
    //    if (startstop_state == LOW) { // pullup -> button pressed if LOW
    //
    //    }
  }
  if (startstop_state == 0) {
    if (!startstop_button_up_old) {
      startstop_button_up_old = true;
      return true;
    }
    return false;
  }
  startstop_button_up_old = false;
  return false;
}

const int speed_max = 1023;
const int speed_step = speed_max / 10;

void apply_current_motor_settings(
  int motorDirection // -1/left, 1/right or 0/stop
  , int motorSpeed // [0, speed_max]
) {
  /*
    L293 logic:    EN1,2   1A    2A
                 H       H     L    Motor turns left  (Forward; motorDirection == 1)
                 H       L     H    Motor turns right (Reverse; motorDirection == 0)

    Motor speed:   PWM signal on EN1,2 (490 Hz; digital output value 0..255 for motorSpeed)
  */
  motorSpeed = map(motorSpeed, 0, speed_max, 0, 255);
  {
    if (motorSpeed == 0) motorDirection = 0;

    if (motorDirection == 1)               //Forward
    {
      digitalWrite(MOTOR_DIRA_PIN, 0);
      digitalWrite(MOTOR_DIRB_PIN, 1);
    }
    else if (motorDirection == -1)               //Backward
    {
      digitalWrite(MOTOR_DIRA_PIN, 1);
      digitalWrite(MOTOR_DIRB_PIN, 0);
    }
    else
    {
      // break
      digitalWrite(MOTOR_DIRA_PIN, 0);
      digitalWrite(MOTOR_DIRB_PIN, 0);
    }
    analogWrite(MOTOR_EN_PIN, motorSpeed);
  }
}

long distance_old = 0;
int speed_cur = 0;
int direction_cur = 0;  // 1/car1 up or -1/car1 down or 0/undetermined (e.g. at startup)
int is_running = 0;     // 0/off, 1/running


void loop() {
  Serial.println("");
  delay(500);
  long distance_cur;
  int car_in_station;

  distance_cur = distance_cm();
//  distance_cur = min(50, distance_cur); // limit to 30cm
  //  Serial.print(distance_cur);
  //  if (distance_old > distance_cur) {
  //    Serial.println(" - moving closer");
  //  } if (distance_old < distance_cur) {
  //    Serial.println(" - moving away");
  //  } else {
  //    Serial.println(" - not moving");
  //  }
  //  Serial.println(hcsr04.ToString());
  distance_old = distance_cur;

  bool is_startstop_button_pressed = startstop_button_pressed();
  car_in_station = determine_car_in_station(); // -1: no car detected, else 0-based car number (0,1)
  //  Serial.print("car in station: ");
  //  Serial.print(car_in_station == -1 ? "-" : String(car_in_station));
  //  Serial.print(", distance: ");
  //  Serial.print(distance_cur);

  if (!is_startstop_button_pressed && !is_running) {
    // do nothing
    //    return;
  } else if (is_startstop_button_pressed && is_running) {
    // stop requested, no matter where we are, stop
    speed_cur = 0;
    is_running = 0;
  } else if (is_startstop_button_pressed) {
    is_running = 1; // TBD
    if (direction_cur == 0) {
      // set direction based on current car in station if any, otherwise choose default direction
      direction_cur = car_in_station;
      if (direction_cur == 0) {
        direction_cur = -1;
      }
    }
  }

  if (is_running) {
    speed_cur = min(speed_max, speed_cur + speed_step);
    if (distance_cur < 20) {
      speed_cur = max(2*speed_step, speed_cur - 2*speed_step);
    }
  }
  if (car_in_station == -direction_cur) { // car detected coming into station -> stop!
    speed_cur = 0;
    is_running = 0;
    direction_cur = 0;
  }

  //  apply_current_motor_settings(direction_cur, speed_cur);
  Serial.print("is_running:");
  Serial.print(is_running);
  Serial.print(", car_in_station:");
  Serial.print(car_in_station);
  Serial.print(", is_startstop_button_pressed:");
  Serial.print(is_startstop_button_pressed);
  Serial.print(", distance_cur:");
  Serial.print(distance_cur);
  Serial.print(", direction_cur:");
  Serial.print(direction_cur);
  Serial.print(", speed_cur:");
  Serial.print(speed_cur);
}
