#include <hcsr04.h>
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


#define RST_PIN         9          // Configurable, see typical pin layout above
#define SS_PIN          10         // Configurable, see typical pin layout above
#define TRIG_PIN        7          // ultrasonic trig
#define ECHO_PIN        8          // ultrasonic echo

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance
HCSR04 hcsr04(TRIG_PIN, ECHO_PIN, 20, 4000); // ultrasonic sensor instance

void setup() {
  Serial.begin(9600);   // Initialize serial communications with the PC
  while (!Serial);    // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
  SPI.begin();      // Init SPI bus
  mfrc522.PCD_Init();   // Init MFRC522
  mfrc522.PCD_SetAntennaGain(MFRC522::PCD_RxGain::RxGain_max);
  mfrc522.PCD_AntennaOn();
  delay(4);       // Optional delay. Some board do need more time after init to be ready, see Readme
  mfrc522.PCD_DumpVersionToSerial();  // Show details of PCD - MFRC522 Card Reader details
  Serial.println(F("Scan PICC to see UID, SAK, type, and data blocks..."));
}

long distanceMillsAvg(HCSR04 hcsr04, int waitMillis, int count) {
  long min, max, avg, d;
  min = 9999;
  max = 0;
  avg = d = 0;

  if (waitMillis < 1) {
    waitMillis = 1;
  }

  if (count < 1) {
    count = 1;
  }

  for (int x = 0; x < count + 2; x++) {
    d = hcsr04.distanceInMillimeters() / 2;

    if (d < min) {
      min = d;
    }

    if (d > max) {
      max = d;
    }

    avg += d;
    delay(waitMillis);
  }

  // substract highest and lowest value
  avg -= (max + min);
  // calculate average
  avg /= count;
  return avg * 2;
}

long distance_old = 0;
const int speed_max = 100;
const int speed_steps = speed_max / 10;
int speed_cur = 0;
int direction_cur = -1; // 1 or -1
int is_running = 0;

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
  for (int i = 0; i < 2; i++) {
    if (is_uid(7, mfrc522.uid.uidByte, car_uid[i])) {
    return i;
  }
}
return -1;
}

void loop() {
  Serial.println("");
  delay(500);
  long distance_cur;
  int car_in_station;

  distance_cur = distanceMillsAvg(hcsr04, 1, 5);
//  Serial.print(distance_cur);
  if (distance_old > distance_cur) {
    //    Serial.println(" - moving closer");
  } if (distance_old < distance_cur) {
    //    Serial.println(" - moving away");
  } else {
    //    Serial.println(" - not moving");
  }
  //  Serial.println(hcsr04.ToString());

  car_in_station = -1; // -1: no car detected, else 0-based car number (0,1)
  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  mfrc522.PCD_AntennaOn();
  if ( mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    Serial.print("detected car");
    dump_uid(7, mfrc522.uid.uidByte);
    Serial.println();
    car_in_station = check_car_in_station(mfrc522.uid.uidByte);
  }
  mfrc522.PCD_AntennaOff();
  Serial.print("car in station: ");
  Serial.print(car_in_station == -1 ? "-" : String(car_in_station));
  Serial.print(", distance: ");
  Serial.print(distance_cur);
  Serial.print(", prev distance: ");
  Serial.print(distance_old);

  distance_old = distance_cur;
}
