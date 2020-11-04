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
uint8_t state,mode;
uint16_t address;

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

void pattern5(){
};
/**
 * @brief odczytuje położenie enkodera
 * 
 */
void readEnc() {
  encPos += encoder.getValue();

  if (encPos > 508) encPos = 1;
  if (encPos < 1) encPos = 508;
  if (encPos != oldEncPos) {
    oldEncPos = encPos;
     display.printNumber(oldEncPos,true); // Expect: A031
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
        display.blink();
        EEPROM.write(0, encPos);
        EEPROM.write(1, encPos >> 8);
        address = encPos;
        break;}

      case ClickEncoder::DoubleClicked:{ //6
        display.clear();
        display.print("FirE");
        break;}
    }
  }
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

  address = (EEPROM.read(1) << 8) + EEPROM.read(0);

  Timer1.initialize(1000);
  Timer1.attachInterrupt(timerIsr);

  encoder.setAccelerationEnabled(true);
  display.begin();            // initializes the display
  display.setBacklight(50);  // set the brightness to 100 %
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
    if (DMXSerial.read(address) == 170) { //safety channel 10101010 
      mode = DMXSerial.read(address+1);
      if (mode > 200) state = true; else state = false; 

      if (DMXSerial.read(address+2) > 0) {
        time = 20*DMXSerial.read(address+2);
       } else {
        time = 1000;
       }

      if (state != currState) {
        currState = state;
        if (state == true) {
          if (mode >= 200 && mode < 210){pattern1();};
          if (mode >= 210 && mode < 220){pattern2();};
          if (mode >= 220 && mode < 230){pattern3();};
          if (mode >= 230 && mode < 240){pattern4();};
          if (mode >= 240 && mode < 250){pattern5();};

          if (mode > 249){
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