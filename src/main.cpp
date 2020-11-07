#include <DMXSerial.h>
#include <Arduino.h>
#include <timeout.h>
#include <SevenSegmentTM1637.h>
#include <SevenSegmentExtended.h>
#include <ClickEncoder.h>
#include <TimerOne.h>
#include <EEPROM.h>

#define VALVE_PIN A0
#define SPARK_PIN A1
#define LED_PIN   13
#define DISPLAY_CLK 2
#define DISPLAY_DIO 3
#define ENCODER_A 5
#define ENCODER_B 6
#define ENCODER_BTN 7
#define MANUAL1_PIN 8
#define MANUAL2_PIN 9
#define MANUAL3_PIN 10  
#define MANUAL4_PIN 11
#define STEPS 4

ClickEncoder encoder(ENCODER_A, ENCODER_B, ENCODER_BTN, STEPS);

Timeout timeoutValve;
Timeout timeoutSpark;


SevenSegmentExtended display(DISPLAY_CLK, DISPLAY_DIO);

int16_t oldEncPos, encPos;
uint8_t buttonState;
uint8_t prevState = -1;
uint8_t currState = -1;
uint16_t time = 1000;
uint8_t dmxState,dmxMode,menuMode,manState,manAddress;
uint16_t dmxAddress;

void timerIsr() {
  encoder.service();
}

/**
 * @brief hujowa procedura
 * 
 */
void pattern1(){
  digitalWrite(VALVE_PIN, HIGH);
  delay(500);
  digitalWrite(SPARK_PIN, HIGH);
  delay(200);
  digitalWrite(VALVE_PIN, LOW);
  digitalWrite(SPARK_PIN, LOW);
}

void pattern2(){
  for (int i = 0; i <= 5; i++) {
    digitalWrite(VALVE_PIN, HIGH);
    delay(200);
    digitalWrite(SPARK_PIN, HIGH);
    delay(200);
    digitalWrite(VALVE_PIN, LOW);
    digitalWrite(SPARK_PIN, LOW);
    delay(200);
  }
}

void pattern3(){
};

void pattern4(){
};
/**
 * @brief patern 5
 * 
 */
void pattern5(){
};

/**
 * @brief Wyswietla menu na wyswietlaczu 7segmentowym
 * 
 * @param number numer
 * @param mode prefix numeru
 */
void printMenu(int16_t number,uint8_t mode) {
  // clear the display first
  display.clear();
  if (mode == 1) {
    if (number > 999) {
      display.setCursor(0, 0);
    } else if (number > 99) {
      display.print('A');
      display.setCursor(0, 1);
    } else if (number > 9) {
      display.print(F("A0"));
      display.setCursor(0, 2);
    } else {
      display.print(F("A00"));
      display.setCursor(0, 3);
    }
  }
  if (mode == 2) {
    display.print(F("Ch "));
    display.setCursor(0, 3);
  }
  display.print(number);
}

void switchMenu(){
  display.blink();
  if (menuMode == 1){
    EEPROM.write(0, encPos);
    EEPROM.write(1, encPos >> 8);
    dmxAddress = encPos;
    encPos = manAddress;
    }
  if (menuMode == 2){
    EEPROM.write(3, encPos);
    manAddress = encPos;
    encPos = dmxAddress;
    }
  menuMode++;
}

/**
 * @brief odczytuje położenie enkodera
 * 
 */
void readEnc() {
  if (menuMode > 2) menuMode = 1;
  if (menuMode < 1) menuMode = 2;

  switch (menuMode){
    case 1:{
      encPos += encoder.getValue();
      if (encPos > 508) encPos = 1;
      if (encPos < 1) encPos = 508;
      if (encPos != oldEncPos) {
       oldEncPos = encPos;
         printMenu(oldEncPos,menuMode); // Expect: A031
      }
      break;
    }
    case 2:{

      encPos += encoder.getValue();
      if (encPos > 4) encPos = 1;
      if (encPos < 1) encPos = 4;
      if (encPos != oldEncPos) {
       oldEncPos = encPos;
         printMenu(oldEncPos,menuMode); // Expect: A031
      }
      break;
    }
  }
  
  buttonState = encoder.getButton();

  if (buttonState != 0) {
    switch (buttonState) {
      case ClickEncoder::Open:          //0
        break;

      case ClickEncoder::Closed:        //1
        break;

      case ClickEncoder::Pressed:       //2
        break;

      case ClickEncoder::Held:{          //3
        display.clear();
        display.print("HELd");
        break;}

      case ClickEncoder::Released:      //4
        break;

      case ClickEncoder::Clicked:{       //5
        switchMenu();
        break;}

      case ClickEncoder::DoubleClicked:{ //6
        display.clear();
        display.print("FirE");
        break;}
    }
  }
}

void fire(){
  timeoutValve.prepare(time);
  timeoutValve.reset();
  timeoutSpark.reset();
}

void setup () {
  DMXSerial.init(DMXReceiver);

  // set some default values
  DMXSerial.write(1, 0);
  DMXSerial.write(2, 0);
  DMXSerial.write(3, 0);
  
  pinMode(VALVE_PIN, OUTPUT); // sets the digital pin as output
  pinMode(SPARK_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(4, OUTPUT);
  digitalWrite(4,HIGH);
  timeoutValve.prepare(1000);
  timeoutSpark.prepare(200);

  encPos = 1;
  menuMode = 1;

  dmxAddress = (EEPROM.read(1) << 8) + EEPROM.read(0);
  encPos = dmxAddress;
  manAddress = EEPROM.read(3);

  Timer1.initialize(1000);
  Timer1.attachInterrupt(timerIsr);

  encoder.setAccelerationEnabled(true);
  display.begin();            // initializes the display
  display.setBacklight(50);  // set the brightness to 100 %
  display.print("   WYrZuTnIA OGNIA    ");
  delay(1000);                // wait 1000 ms
  display.print("InIt");      // display INIT on the display
  delay(1000);                // wait 1000 ms
}

void loop() {
  // Calculate how long no data backet was received
  readEnc();
  unsigned long lastPacket = DMXSerial.noDataSince();
  digitalWrite(VALVE_PIN, !timeoutValve.time_over());
  digitalWrite(SPARK_PIN, !timeoutSpark.time_over());

  if (lastPacket < 500) {
    digitalWrite(LED_PIN, HIGH);
    if (DMXSerial.read(dmxAddress) == 170) { //safety channel 10101010 
      dmxMode = DMXSerial.read(dmxAddress+1);
      if (dmxMode > 200) dmxState = true; else dmxState = false; 

      if (DMXSerial.read(dmxAddress+2) > 0) {
        time = 20*DMXSerial.read(dmxAddress+2);
       } else {
        time = 1000;
       }

      if (dmxState != currState) {
        currState = dmxState;
        if (dmxState == true) {
          if (dmxMode >= 200 && dmxMode < 210){pattern1();};
          if (dmxMode >= 210 && dmxMode < 220){pattern2();};
          if (dmxMode >= 220 && dmxMode < 230){pattern3();};
          if (dmxMode >= 230 && dmxMode < 240){pattern4();};
          if (dmxMode >= 240 && dmxMode < 250){pattern5();};

          if (dmxMode > 249){
            timeoutValve.prepare(time);
            timeoutValve.reset();
            timeoutSpark.reset();
          };
          }
        }
    }
  } else {
    // Show pure red color, when no data was received since 5 seconds or more.
    digitalWrite(LED_PIN, LOW);
    digitalWrite(VALVE_PIN, LOW);
    digitalWrite(SPARK_PIN, LOW);
  } // if*/
}
// End.