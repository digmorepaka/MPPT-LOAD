#include <Arduino.h>
#include <Adafruit_ADS1X15.h>
#include <Adafruit_MCP4725.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <SD.h>


LiquidCrystal_I2C lcd(0x27,20,4);
Adafruit_MCP4725 dac;
Adafruit_ADS1115 ads; 
File myFile;

Sd2Card card;
SdVolume volume;
SdFile root;

float deltaI, deltaU, setVal, current, current2, voltage, watt, lastcurrent, lastvoltage, lastwatt, wh, ah;
float delta = 1;
byte lpmode = 0, rampon = 0;
int  ui, update = 1, page = 10, lastpage, mode = 0, counts, lastSet, rampcounts = 0;
unsigned long int lastpressed, lastscreen, Tramp, x, y;



void press()
{
  
  if (millis() - lastpressed > 150)
  {
    if (digitalRead(8) == 0) //esc
    {
      if(mode >= 1){
        mode = 0;
        update = 1;
      }
    }
    else if (digitalRead(6) == 0) //up
    {
      if (mode == 0){
        if(page == 0){
          page = 5;
        }
        else{
          page--;
        }
        lastpressed = millis();
      }else if(mode == 2 && page == 1){
        counts = counts + 1;
      }
    }
    else if (digitalRead(9) == 0) //down
    {
      if (mode == 0){
        page++;
        lastpressed = millis();
      }else if(mode == 2 && page == 1){
        counts = counts - 1;
      }
    }
    else if (digitalRead(4) == 0) //right
    {
      Serial.println("4");
      
    }
    else if (digitalRead(5) == 0) //enter
    {
      if(page == 0 && mode == 0){
        mode = 1;
        update = 1;

      }else if(page == 1 && mode == 0){
        mode = 2;
        update = 1;

      }else if(page == 2 && mode == 0){
        mode = 3;
        update = 1;

      }else if(page == 3 && mode == 0){
        mode = 4;
        update = 1;

      }else if(page == 4 && mode == 0){
        mode = 5;
        update = 1;

      }else if(page == 5 && lpmode == 0 && mode == 0){
        mode = 0;
        lpmode = 1;
        update = 1;
      }else if(page == 5 && lpmode == 1 && mode == 0){
        mode = 0;
        lpmode = 0;
        update = 1;
      }
    }
    lastpressed = millis();
  }
}


void setup() {
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("startup");
  if(!dac.begin(0x61)){
    lcd.clear();
    lcd.print("DAC Error.");
    for(;;);
  }
  
  dac.setVoltage(0, true);

  if (!ads.begin()) {
    lcd.clear();
    lcd.print("ADC Error.");
    for(;;);
  }
  //                                                                ADS1015  ADS1115
  //                                                                -------  -------
  // ads.setGain(GAIN_TWOTHIRDS);  // 2/3x gain +/- 6.144V  1 bit = 3mV      0.1875mV (default)
  // ads.setGain(GAIN_ONE);        // 1x gain   +/- 4.096V  1 bit = 2mV      0.125mV
  // ads.setGain(GAIN_TWO);        // 2x gain   +/- 2.048V  1 bit = 1mV      0.0625mV
  ads.setGain(GAIN_FOUR);       // 4x gain   +/- 1.024V  1 bit = 0.5mV    0.03125mV
  // ads.setGain(GAIN_EIGHT);      // 8x gain   +/- 0.512V  1 bit = 0.25mV   0.015625mV
  // ads.setGain(GAIN_SIXTEEN);    // 16x gain  +/- 0.256V  1 bit = 0.125mV  0.0078125mV
  /*
  pinMode(2, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(2), addPage, FALLING);
  pinMode(3, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(3), addUI, FALLING);
  pinMode(4, OUTPUT); //LP mode
  */
  pinMode(2, INPUT_PULLUP);
  pinMode(4, INPUT_PULLUP);
  pinMode(5, INPUT_PULLUP);
  pinMode(6, INPUT_PULLUP);
  pinMode(7, INPUT_PULLUP);
  pinMode(8, INPUT_PULLUP);
  pinMode(9, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(2), press, FALLING);

  pinMode(10, OUTPUT);
  SD.begin(10);
  
  if (!volume.init(card)) {
    lcd.clear();
    lcd.print("SD error");
    delay(200);
    //while (1);
  }

  Serial.begin(115200);
  /*pinMode(encoder0PinA, INPUT);
  digitalWrite(encoder0PinA, HIGH);       // turn on pull-up resistor
  pinMode(encoder0PinB, INPUT);
  digitalWrite(encoder0PinB, HIGH);       // turn on pull-up resistor
  attachInterrupt(0, doEncoder, CHANGE);*/
  lcd.clear();
}


void loop(){

  digitalWrite(A1, lpmode);
  voltage = ads.readADC_SingleEnded(0) * 0.03125 * 101.00;
  switch(lpmode){
    case 0:
      //current = ads.readADC_SingleEnded(1) * 0.03125 / 0.022;
      current = map(ads.readADC_SingleEnded(1), 0, 2944, 0, 3000);
      break;
    case 1:
      current = ads.readADC_SingleEnded(1) * 0.03125 / 0.12;
      break;
  }
  
  current2 = ads.readADC_SingleEnded(2) * 0.03125F;
  //current2 = map(current2, 200.00, 864.00, 20000, 0);
  //current = ads.readADC_SingleEnded(1);
  //current = map(current, 128, 13168, 47, 2994);


  watt = voltage * current / 1000;
  
  if(millis() - y >= 100){
    if(mode > 0){      
      wh = wh + watt / 1000.00 / 36000.00;
      ah = ah + current / 1000.00 / 36000.00;
    } else if(mode == 0){
      wh = 0;
      ah = 0;
    }
    y = millis();
  }

  switch(mode){
    case 0: //idle
      dac.setVoltage(0, true);
      lastSet = 0;
      counts = 0;
      break;
    case 1: //MPPT POB
      if (millis() - lastSet > 200){
        deltaI = current - lastcurrent;
        deltaU = voltage - lastvoltage;
        if (watt >= lastwatt)
        { 
          if(voltage > 100){
           counts = counts + delta;
          }
        }
        else if(watt < lastwatt)
        {
          
          if (voltage < 500){
            switch(lpmode){
              case 0:
                counts = counts - 5;
                break;
              case 1:
                counts = counts - 10;
                break;
            }
          }else if (voltage < lastvoltage){
            counts = counts - delta;}
        }
        lastcurrent = current;
        lastvoltage = voltage;
        lastwatt = watt;
        if(counts > 3720)
        counts=3720;
        if (counts < 1)
          counts=1;
        dac.setVoltage(counts, true);
        lastSet = millis();
      }
      break;
    case 2: //CC
      //counts = map(encoder0Pos, 0, 65535, 0, 65535);
      if(voltage < 1000){
        dac.setVoltage(0, true);
        rampon = 1;
      }else if(rampon == 1){
          if(rampcounts < counts && millis() - Tramp > 50){
            rampcounts++;
            dac.setVoltage(rampcounts, true);
            Tramp = millis();
          } else if(rampcounts == counts){
            rampon = 0;
            rampcounts = 0;
            Tramp = 0;
          }
      }else if(counts != lastSet && rampon == 0){
        dac.setVoltage(counts, true);
        lastSet = counts;
      }
      break;
    case 3: //CP
      if(watt > 10000){
      if(voltage > lastvoltage){
        deltaU = voltage / lastvoltage;
        counts = counts * deltaU;
      }
        else if(voltage <= lastvoltage){
         deltaI = current / (1000 / voltage);
          counts = current / deltaI;
        }
      
      }
      else if(watt < 1000){
        if(voltage < lastvoltage){
         deltaU = lastvoltage / voltage;
         counts = counts * deltaU;
        } else if(voltage >= lastvoltage){
         deltaI = current / (1000 / voltage);
         counts = current / deltaI;
        }
        }else if(watt == 10000){
          //nothing to do
      }
      lastcurrent = current;
      lastvoltage = voltage;
      lastwatt = watt;
      break;
    case 4: //Battery
      break;
    case 5: //CR
      break;
    

  }
  
  if (mode > 0 && millis() - lastscreen > 500){
    update = 1;
  }

  if(page != lastpage || update){
    lcd.clear();
    switch(page){
      case 0:
        switch(mode){
          case 0:
            lcd.setCursor(0,0);
            lcd.print("MPPT");
            lcd.setCursor(0,1);
            lcd.print("Constant Current");
            lastpage = page;
            break;
          case 1:
            lcd.setCursor(0,0);
            lcd.print(voltage / 1000.0);
            lcd.print("V ");
            lcd.print(current / 1000.0);
            lcd.print("A ");
            lcd.setCursor(0,1);
            lcd.print(watt / 1000);
            lcd.print("W ");
            lcd.print(wh);
            lcd.print("Wh ");
            lastpage = page;
            break;
          default:
            break;

        }
        
        break;
      case 1:
      switch(mode){
          case 0:
            lcd.setCursor(0,0);
            lcd.print("Constant Current");
            lcd.setCursor(0,1);
            lcd.print("Constant Power");
            lastpage = page;
            break;
          case 2:
            lcd.setCursor(0,0);
            lcd.print(current / 1000.0);
            lcd.print("A ");
            lcd.print(ah);
            lcd.print("Ah ");
            lcd.setCursor(0,1);
            lcd.print(watt  / 1000);
            lcd.print("W ");
            lcd.print(wh);
            lcd.print("Wh ");
            lastpage = page;
            break;
          default:
            break;

        }
        break;
      case 2:
        lcd.setCursor(0,0);
        lcd.print("Constant Power");
        lcd.setCursor(0,1);
        lcd.print("Battery");
        lastpage = page;
        break;
      case 3:
        lcd.setCursor(0,0);
        lcd.print("Battery");
        lcd.setCursor(0,1);
        lcd.print("Constant Resistance");
        lastpage = page;
        break;
      case 4:
        lcd.setCursor(0,0);
        lcd.print("Constant Resistance");
        lcd.setCursor(0,1);
        switch(lpmode){
              case 0:
                lcd.print("LP Mode");
                break;
              case 1:
                lcd.print("*LP Mode");
                break;
        }
        lastpage = page;
        break;
      case 5:
        lcd.setCursor(0,0);
        switch(lpmode){
              case 0:
                lcd.print("LP Mode   ");
                break;
              case 1:
                lcd.print("*LP Mode");
                break;
        }
        lastpage = page;
        break;
      default:
        lastpage = page;
        page = 0;
        break;


    }
    lastscreen = millis();
    update = 0;
  }
  if (millis() - x > 100){
    //Serial.print("Steps: ");
    //Serial.print(encoder0Pos);
    Serial.print("Counts: ");
    Serial.print(counts);
    Serial.print(" Voltage: ");
    Serial.print(voltage / 1000.00);
    Serial.print("V");
    Serial.print(" ADC: ");
    Serial.print(ads.readADC_SingleEnded(1));
    Serial.print(" Amps: ");
    Serial.print(current);
    Serial.print("mA");
    Serial.print(" Sec shunt: ");
    Serial.print(current2);
    Serial.println("mV");

    
    x = millis();
  }
}
