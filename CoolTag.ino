/***********************************************************************
Copyright (c) 2020-2021 Ali Bohloolizamani

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 ***********************************************************************/
#include <Adafruit_GFX.h>     // https://github.com/adafruit/Adafruit-GFX-Library
#include <Adafruit_SSD1306.h> // https://github.com/adafruit/Adafruit_SSD1306
#include <EEPROM.h>           // https://github.com/arduino/ArduinoCore-avr/blob/master/libraries/EEPROM/src/EEPROM.h
#include <SoftwareSerial.h>   // https://github.com/lstoll/arduino-libraries/blob/master/SoftwareSerial/SoftwareSerial.h
#include <Wire.h>             // https://github.com/arduino/ArduinoCore-avr/blob/master/libraries/Wire/src/Wire.h

// SHT30 I2C address is 0x45(69)
#define Addr 0x45
#define GLEDPIN 13
#define YLEDPIN 2
#define PBPIN 3
#define BTEPIN 6
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define SRXDPIN 4
#define STXDPIN 5
#define LCD_ADDRESS 0x3c

float t, h;
unsigned int pBCnt = 0;
unsigned int writeAdrs = 0x0000;
unsigned int readAdrs = 0x0000;
boolean next = 0;
bool sample = 0;
bool sendFlag = 0;
bool writeDone = 0;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
SoftwareSerial debug(SRXDPIN, STXDPIN);

ISR(TIMER1_COMPA_vect) {
  if (next) {
    digitalWrite(GLEDPIN, HIGH);
    next = 0;
  } else {
    digitalWrite(GLEDPIN, LOW);
    next = 1;
  }
  sample = 1;
}

void updateDisplay(float t, float h) {
  display.clearDisplay();
  display.setCursor(1, 5);
  display.print("Temperature: ");
  display.print(t);
  display.println(" \337C");
  display.setCursor(10, 20);
  display.print("Humidity: ");
  display.print(h);
  display.println(" %RH");
  display.display();
}

void setup() {
  Serial.begin(9600); // BLE
  debug.begin(9600); // Debug logs
  pinMode(BTEPIN, OUTPUT);
  digitalWrite(BTEPIN, LOW); // Disable the BLE for now
  float battery = 10.0 * analogRead(A0) / 1024;
  // Initialise I2C communication as MASTER
  Wire.begin();
  display.begin(SSD1306_SWITCHCAPVCC, LCD_ADDRESS);
  delay(100);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.cp437(true);
  display.setCursor(5, 15);
  display.print("Battery ");
  display.print(battery);
  display.print(" Volt");
  display.display();
  debug.print("\nBattery Voltage: ");
  debug.println(battery);
  delay(3000);
  display.clearDisplay();
  display.setCursor(10, 8);
  debug.println("CoolTag");
  display.println("CoolTag");
  display.setCursor(25, 20);
  debug.println("Copyright 2021");
  display.println("Copyright 2021");
  display.display();
  pinMode(GLEDPIN, OUTPUT);
  pinMode(YLEDPIN, OUTPUT);
  pinMode(PBPIN, INPUT_PULLUP);
  digitalWrite(YLEDPIN, HIGH);
  while (writeAdrs != 0x400) {
    EEPROM.update(writeAdrs++, 0x00);
    EEPROM.update(writeAdrs++, 0x00);
  }
  writeAdrs = 0;
  delay(1000);
  digitalWrite(YLEDPIN, LOW);
  cli(); // Stop interrupts
  // Set timer1 interrupt at 1Hz
  TCCR1A = 0; // Set entire TCCR1A register to 0
  TCCR1B = 0; // Same for TCCR1B
  TCNT1  = 0; // Initialize counter value to 0
  // Set compare match register for 1hz increments
  OCR1A = 3905; // (4*10^6) / (1*1024) - 1 (must be <65536)
  // Turn on CTC mode
  TCCR1B |= (1 << WGM12);
  // Set CS10 and CS12 bits for 1024 prescaler
  TCCR1B |= (1 << CS12) | (1 << CS10);
  // Enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);
  sei(); // Allow interrupts
}

void loop() {
  float t_temp, h_temp;

  if (sample == 1) {
    unsigned int data[6];
    // Start I2C Transmission
    Wire.beginTransmission(Addr);
    // Send measurement command
    Wire.write(0x2C);
    Wire.write(0x06);
    // Stop I2C transmission
    Wire.endTransmission();
    delay(500);
    // Request 6 bytes of data
    Wire.requestFrom(Addr, 6);
    // Read 6 bytes of data
    // temp msb, temp lsb, temp crc, humidity msb, humidity lsb, humidity crc
    if (Wire.available() == 6) {
      data[0] = Wire.read();
      data[1] = Wire.read();
      data[2] = Wire.read();
      data[3] = Wire.read();
      data[4] = Wire.read();
      data[5] = Wire.read();
    }
    // Convert the data
    t_temp = ((((data[0] * 256.0) + data[1]) * 175) / 65535.0) - 45;
    h_temp = ((((data[3] * 256.0) + data[4]) * 100) / 65535.0);

    if (!isnan(t_temp) && !isnan(h_temp)) {
      t = t_temp;
      h = h_temp;
      updateDisplay(t, h);
      debug.print("\nT: ");
      debug.println(t);
      debug.print("H: ");
      debug.println(h);
      if (writeDone == 0) {
        debug.print("EEPROM @ ");
        debug.println(writeAdrs);
        EEPROM.update(writeAdrs++, (byte) h);
        EEPROM.update(writeAdrs++, (byte) ((int) (h * 100) % 100));
        EEPROM.update(writeAdrs++, (byte) t);
        EEPROM.update(writeAdrs++, (byte) ((int) (t * 100) % 100));
        if (writeAdrs == 1024) {
          writeDone = 1;
          writeAdrs = 0;
        }
      }
      sample = 0;
    }
  }
  if (digitalRead(PBPIN) == LOW) {
    digitalWrite(BTEPIN, HIGH); // Enable the BLE
    pBCnt++;
  } else {
    pBCnt = 0;
  }
  if (pBCnt == 0xfff) {
    sendFlag = 1;
  }
  if (Serial.available()) {
    if (Serial.readStringUntil('\n') == "CoolTag") {
      sendFlag = 1;
    }
  }
  if (sendFlag == 1) {
    digitalWrite(YLEDPIN, HIGH);
    readAdrs = 0;
    while (readAdrs != 0x400) {
      char temp = EEPROM.read(readAdrs++) + ' ';
      Serial.print(temp);
    }
    digitalWrite(YLEDPIN, LOW);
    sendFlag = 0;
  }
}
