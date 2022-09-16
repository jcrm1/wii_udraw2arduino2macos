//#include <AbsMouse.h>

//  ii_udraw2arduino2linux
//  The Arduino portion of the code
//  Modified to emulate WACOM IV by jcrm1
//  Created by Matthew Watts 2018
//
//  Tablet wiring differs from normal nunchuck wiring
//  Yellow > A5/SCL
//  Green > A4/SDA
//  Blue > GND
//  Brown > 5v
//
//  With thanks to everybody who has already published
//  code to use wii nunchuck for the init sequences
//  The tablet data was decoded by trial and error

// udraw responds to address I2C 0x52
#include <Wire.h>

// global most up to date tablet state
uint16_t x_raw;
uint16_t y_raw;
//uint16_t x_raw_old = 0;
//uint16_t y_raw_old = 0;
uint8_t pressure_raw;
uint8_t stylusb2;
uint8_t stylusb1;
uint8_t stylusb0;
bool sending = true;
// Setup serial and send init to tablet
void setup() {
//  AbsMouse.init(5120, 4880);
  Serial.begin(9600);
  Wire.begin();
  Wire.beginTransmission(0x52);
  Wire.write(0xF0);
  Wire.write(0x55);
  Wire.endTransmission();
  delay(100);
  Wire.beginTransmission(0x52);
  Wire.write(0xFB);
  Wire.write(0x00);
  Wire.endTransmission();
  delay(5);
}

// get data from tablet and check if state has
// altered from last run
uint8_t hasChange = 0;
uint8_t reply[10];
uint8_t lastReply[10];
uint8_t firstRun = 1;
void getDataFromTablet() {
  Wire.beginTransmission(0x52);
  Wire.write(0x00);
  Wire.endTransmission();
  Wire.requestFrom(0x52, 6);
  
  // Read bytes
  hasChange = 0;
  unsigned long timeout=millis();
  for(int lp=0; lp<6; lp++) {
    while( !Wire.available() ) {
      if( millis() - timeout > 2 ) {
        return;
      } 
    };
    reply[lp] = (uint8_t)Wire.read();
    if( firstRun == 1 ) {
      lastReply[lp] = reply[lp];
    } else {
      if(lastReply[lp] != reply[lp]) {
        lastReply[lp] = reply[lp];
        hasChange = 1;
      }
    }
  }
  firstRun = 0;
  
  // decode
  // the x and y are 12bit values that are
  // mixed together in the first 3 bytes of the
  // data
  uint8_t x_nibble_one = reply[2] & 0x0F;  // 2R
  uint8_t x_nibble_two = reply[0] >> 4;    // 0L
  uint8_t x_nibble_three = reply[0] & 0x0F;// 0R
  
  uint8_t y_nibble_one = reply[2] >> 4;    // 2L
  uint8_t y_nibble_two = reply[1] >> 4;    // 1L
  uint8_t y_nibble_three = reply[1] & 0x0F;// 1R
 
  x_raw = (uint16_t)x_nibble_one<<8 | (uint16_t)x_nibble_two<<4 | (uint16_t)x_nibble_three;
  y_raw = (uint16_t)y_nibble_one<<8 | (uint16_t)y_nibble_two<<4 | (uint16_t)y_nibble_three;

  // pressure is an 8bit value from 4th byte
  pressure_raw = reply[3];

  // byte 5 contains 3 digital button states
  // the two buttons on the side of the pen
  // and one that comes on when pressure is high
  stylusb2 = !(reply[5] & 1);
  stylusb1 = !(reply[5]>>1 & 1);
  stylusb0 = (reply[5]>>2 & 1);
}

// send out data via serial
// only mode 0 is implemented here
// but there is room to add modes later
// for ISDV4 serial tablet emulation
uint8_t serialMode=2;
void sendData() {
  if (serialMode == 0) {
      Serial.write(x_raw);
      Serial.write(y_raw);
      Serial.write(pressure_raw);
      Serial.write(stylusb0);
      Serial.write(stylusb1);
      Serial.write(stylusb2);
      Serial.flush();
  } else if (serialMode == 1) {
      Serial.print(x_raw);
      Serial.print(" ");
      Serial.print(y_raw);
      Serial.print(" ");
      Serial.print(pressure_raw);
      Serial.print(" ");
      Serial.print(stylusb0);
      Serial.print(" ");
      Serial.print(stylusb1);
      Serial.print(" ");
      Serial.print(stylusb2);
      Serial.println("");
      Serial.flush();
  } else if (serialMode == 2) {
      uint8_t firstByte = 0b10100000;
      if (x_raw != 4095) firstByte += 0b01000000;
      if (stylusb0 == 1 || stylusb1 == 1 || stylusb2 == 1) firstByte += 0b00001000;
      firstByte += (x_raw >> 14) & 0b00000011;
      uint8_t secondByte = (x_raw >> 7) & 0b01111111;
      uint8_t thirdByte = x_raw & 0b01111111;
      uint8_t fourthByte = (y_raw >> 14) & 0b00000011;
      if (stylusb0 == 1) fourthByte += 0b00001000;
      if (stylusb1 == 1) fourthByte += 0b00010000;
      if (stylusb2 == 1) fourthByte += 0b00100000;
      byte mappedPressure = map(pressure_raw, 8, 255, -128, 128) & 0xFF;
      fourthByte += (mappedPressure << 2) & 0b00000100;
      uint8_t fifthByte = (y_raw >> 7) & 0b01111111;
      uint8_t sixthByte = y_raw & 0b01111111;
      uint8_t seventhByte = mappedPressure >> 1;
      seventhByte += (mappedPressure >> 1) && 0b01000000;
      Serial.write(firstByte);
      Serial.write(secondByte);
      Serial.write(thirdByte);
      Serial.write(fourthByte);
      Serial.write(fifthByte);
      Serial.write(sixthByte);
      Serial.write(seventhByte);
      Serial.flush();
  } else {
    
  }
}

void loop() {
  getDataFromTablet();
  if (sending) sendData();
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\r');
    if (command == "~#") Serial.print("~#UD-1212-R00,V1.2-7\r");
    else if (command == "SP") sending = false;
    else if (command == "ST") sending = true;
    else if (command == "~C") Serial.print("~C02000,01500\r");
    else if (command == "~R") Serial.print("~RE203A000,000,02,2000,1500");
    else if (command == "SETSERIAL1") serialMode = 1;
    else if (command == "SETSERIAL2") serialMode = 2;
    else if (command == "SETSERIAL0") serialMode = 5;
    else Serial.print(command);
  }
}
