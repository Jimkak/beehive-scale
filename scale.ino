#include "HX711.h"
#include <EEPROM.h>
#include <LiquidCrystal.h>
#include <gprs.h>
#include <stdio.h>
#include <SoftwareSerial.h>

int menu = 0;
int prevValue = 0;
long tar1, tar2;
long gdiff;

// DISPLAY PINS
const int rs = 13, en = 10, d4 = 4, d5 = 5, d6 = 6, d7 = 12;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// HX711 PINS
HX711 scale(11, 9); 

// BUTTON PIN
#define MENU_BUTTON 2

// GPRS MODULE
GPRS gprs;
//SoftwareSerial mySerial(8, 7);

void setup() 
{
  Serial.begin(38400);  
//    mySerial.begin(9600);
  pinMode(MENU_BUTTON, INPUT);
  lcd.begin(16, 2);
  lcd.print("Starting up...");

  //READ FROM MEMORY
  tar1 = EEPROMReadlong(0);
  scale.set_offset(tar1);   // reset the scale to 0

  gdiff = EEPROMReadlong(8);
  scale.power_up();

}

void sendSMS(float weight){
  gprs.preInit();
  delay(1000);
  while(0 != gprs.init()) {
      delay(1000);
      Serial.print("init error\r\n");
  } 
  Serial.println("Init success, start to send SMS message...");

  String buf;
  buf += F("Weight: ");
  buf += String(weight/1000, 2);
  char copy[50];
  buf.toCharArray(copy, 50);
  
//  mySerial.write("AT+CPOWD=1"); //Because we want to send the SMS in text mode
//  gprs.sendSMS("+38641215591",copy); //define phone number and text
}

void loop() 
{
  calibrationMenu();
  readScale();
  scale.power_up();
}


void calibrationMenu(){
  
  int sensorValue = digitalRead(MENU_BUTTON);
  if(sensorValue && !prevValue){
    menu++;
  }
  prevValue = sensorValue;

  if(menu > 0){
    switch(menu){
      case 1:
        lcd.setCursor(0,0);
        lcd.print("Release scale.                                 ");
        menu++;
        break;
      case 2:
        delay(100);
        break;
      case 3:
        // WRITE TO MEMORY!!
        tar1 = scale.read();
        
        EEPROMWritelong(0, tar1);
        
        scale.set_offset(tar1);
        Serial.print("Scale tar complete! (");
        Serial.print(tar1);
        Serial.println(")");
        lcd.setCursor(0,0);
        lcd.print("Put 2kg on.        ");
        lcd.setCursor(0,1);
        lcd.print("Press button.      ");
        menu++;
        break;
      case 4:
        delay(100);
        break;
      case 5:
        // SAVE TO EEPROM
        double read1 = scale.read();
        gdiff = abs(((read1) - (tar1)) / 2450);
        
        EEPROMWritelong(8, gdiff);
        menu = 0;
        lcd.setCursor(0,0);
        lcd.print("Setup complete");
        break;
        
    }
  }
}

void readScale(){
  if(menu == 0){
    double r1, r2, r3;
    r1 = scale.read();
    delay(100);
    r2 = scale.read();
    delay(100);
    r3 = scale.read();
    delay(100);
    float weight = abs(((((r1+r2+r3)/3)-(tar1))/gdiff));
    Serial.println(weight/1000);
    
    lcd.setCursor(0,0);
    lcd.print("Weight:            ");
    lcd.setCursor(0,1);
    lcd.print(weight/1000, 2);
    lcd.print("kg              ");

    sendSMS(weight);
    scale.power_down();      
    delay(5000);
  }
}

void EEPROMWritelong(int address, long value)
{
  //Decomposition from a long to 4 bytes by using bitshift.
  //One = Most significant -> Four = Least significant byte
  byte four = (value & 0xFF);
  byte three = ((value >> 8) & 0xFF);
  byte two = ((value >> 16) & 0xFF);
  byte one = ((value >> 24) & 0xFF);
  
  //Write the 4 bytes into the eeprom memory.
  EEPROM.write(address, four);
  EEPROM.write(address + 1, three);
  EEPROM.write(address + 2, two);
  EEPROM.write(address + 3, one);
}

long EEPROMReadlong(long address)
{
  //Read the 4 bytes from the eeprom memory.
  long four = EEPROM.read(address);
  long three = EEPROM.read(address + 1);
  long two = EEPROM.read(address + 2);
  long one = EEPROM.read(address + 3);
  
  //Return the recomposed long by using bitshift.
  return ((four << 0) & 0xFF) + ((three << 8) & 0xFFFF) + ((two << 16) & 0xFFFFFF) + ((one << 24) & 0xFFFFFFFF);
}
