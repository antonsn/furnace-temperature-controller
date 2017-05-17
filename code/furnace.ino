#include <LiquidCrystal.h>
#include <SPI.h>
#include <EEPROM.h>
#include <Average.h>
#include "Adafruit_MAX31855.h"
#include <math.h>

class TempInterval
{
public:
  TempInterval();
  TempInterval(int _from, int _to, int _tempFrom, int _tempTo)
  {
    from = _from;
    to = _to;
    tempFrom = _tempFrom;
    tempTo = _tempTo;
  }

  int from;
  int to;
  int tempFrom;
  int tempTo;
};


/* parameters to change */
const int temperatureSamplesCount = 5;

/*   
 *    Proportional - Calculate how long heater should be ON. For me was a simplest working solution to prevent overshooting.
 *    OnOffSec = max(tempDiff * maxOnTempDiffK, minOnCycleSec);
 *    const int maxOnTempDiffK = 0.3;
 *    if difference is 10 C then OnSec will be 10 * 0.3 = 3 sec  
 *    
 *    TempInterval(from  minute, to  minute, from temp, to temp)
 *    from minute 0 to 7 raise  temperature from 200 to 300
 *     TempInterval(0, 7, 200, 300),
 *    
 *    from minute 7 to 17 hold  temperature 300
 *     TempInterval(7, 17, 300, 300),
 *    
*/

const int atTempDiffStartAdjustTimeOnOff = 20;
const int maxOnTempDiffK = 0.3;
const int minOnOffCycleSec = 1; //avoiding switchng too often 


const int mininumMeaningfullTemperatureC = 5;
const int minAlowedTemperatureChange = 10;
const bool exitOnError = true;

const int TempIntervalCnt = 6;

TempInterval tempIntervals[TempIntervalCnt] =
{
        TempInterval(0, 7, 50, 150),
        TempInterval(7, 15, 150, 200),
        TempInterval(15, 30, 200, 200),
        TempInterval(30, 45, 200, 300),
        TempInterval(45, 60, 300, 150),
        TempInterval(60, 90, 150, 150)
};

Average<double> temperatures(temperatureSamplesCount);

enum States
{
  none,
  finished,
  started,
  notfinished,
  askedToStop

};

States state;

long offDuration;
long onDuration;
int OnOffSec;

int currentTemp = 0;
int targetTemp;
int currentMinute;
long startTime;
int lastExecutionMinute;

volatile long lastOnTime;
volatile long lastOffTime;

bool IsOn = false;
bool switchOn = false;




LiquidCrystal lcd(7, 8, 9, 10, 11, 12);
const int thermoCLK = 3, thermoCS = 4, thermoDO = 5;
Adafruit_MAX31855 thermocouple(thermoCLK, thermoCS, thermoDO);

void setup()
{

  startTime = millis();
  Serial.begin(9600);

  lcd.begin(16, 2);
  lcd.print("Hello");

  delay(2500);

  pinMode(2, OUTPUT);

  lastExecutionMinute = EEPROMReadInt(0);
  if (lastExecutionMinute == -1) //finished
  {
    lastExecutionMinute = 0;
    state = none;
  }
  else
    state = notfinished;
}

void loop()
{

  delay(700);

  for (int i = 0; i < temperatureSamplesCount; i++)
  {

    float currentTemperature = thermocouple.readCelsius();

    if (isnan(currentTemperature))
      Serial.println("Wrong T reading ");
    else
      temperatures.push(currentTemperature);

    if (currentTemperature < mininumMeaningfullTemperatureC)
    {
      Serial.print("Wrong T reading - low");
      Serial.println(currentTemperature);
    }
    else
      temperatures.push(currentTemperature);
  }

  int avgTemp = round(temperatures.mode());

  Serial.print("T avg : ");
  Serial.println(avgTemp);

  if (!isTempChangeOk(avgTemp))
  {
   //exit and switch off
   switchOn = false; setHeater();
   return;
   
  }
  currentTemp = avgTemp;

  currentMinute = ((millis() - startTime) / 60000) + lastExecutionMinute;
  targetTemp = getCurrentInterval(currentMinute);

  int input = readInput();

  if (targetTemp == -1) //finished
  {
    setFinished();
  }

  switch (state)
  {
  case notfinished:

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("P.not done");
    lcd.setCursor(11, 0);
    lcd.print(lastExecutionMinute);

    lcd.setCursor(0, 1);
    lcd.print("N");
    lcd.setCursor(5, 1);
    lcd.print("Y");
    lcd.setCursor(8, 1);
    lcd.print("Contin?");

    if (input == 1)
      state = started;
    if (input == 2)
      state = finished;

    break;

  case none:
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Start P1?  ");
    lcd.setCursor(0, 1);
    lcd.print("N");
    lcd.setCursor(5, 1);
    lcd.print("Y");

    if (input == 1)
      state = started;
    if (input == 2)
      state = none;

    break;

  case askedToStop:

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Stop? ");
    lcd.setCursor(0, 1);
    lcd.print("N");
    lcd.setCursor(5, 1);
    lcd.print("Y");

    if (input == 1)
      state = finished;
    if (input == 2)
      state = started;

    break;

  case started:

    EEPROMWriteInt(0, currentMinute);

    setHeater();

    lcd.clear();
    lcd.setCursor(0, 0);

    lcd.print("Min");
    lcd.print(" Cur.T");
    lcd.print(" Tar.T");

    lcd.setCursor(0, 1);

    lcd.print(currentMinute);

    lcd.setCursor(5, 1);
    lcd.print(currentTemp);
    lcd.setCursor(10, 1);

    lcd.print(targetTemp);

    lcd.setCursor(14, 1);

    if (IsOn)

      lcd.print("On");
    else
      lcd.print("  ");

    if (input > 0)
      state = askedToStop;

    break;

  case finished:

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Finished");
    digitalWrite(2, LOW);

    EEPROMWriteInt(0, -1);
    lastExecutionMinute = -1;

    if (input > 0)
    {
      state = none;
      setup();
    }
    break;
  }
}

long pressed;

bool isTempChangeOk(int newTemp)
{
  
  if (currentTemp != 0) {
      if (abs(newTemp) - abs(currentTemp) > minAlowedTemperatureChange)
      {
  
        Serial.print("T changed too quickly from ");
        Serial.println(currentTemp);
        Serial.print("to ");
        Serial.println(newTemp);
  
        lcd.clear();
        lcd.print("T too quickly");
  
        if (exitOnError)
        {
          setFinished();
          
          lcd.clear();
          lcd.print("T too quickly - Exit");
          Serial.println("exiting app");
        }
  
        return false;
      }  

     if (currentTemp < mininumMeaningfullTemperatureC)
     {
        lcd.clear();
        lcd.print("T is too low");
        return false;
     }
  }
  return true;

}

void setFinished()
{
    state = finished;
    EEPROMWriteInt(0, -1);
}


int readInput()
{

  int inputGreen = analogRead(3);
  int inputBlack = analogRead(1);

  if (inputGreen == 0 || inputBlack == 0)
    pressed++;
  else
    pressed = 0;

  if (pressed > 2) //long btn hold
  {

    pressed = 0;

    if (inputGreen == 0)
      return 1;

    if (inputBlack == 0)
      return 2;
  }

  return 0;
}

void setHeater()
{

  if (IsOn)
    lastOnTime = micros();
  else
    lastOffTime = micros();

  offDuration = (micros() - lastOnTime) / 1000000;
  onDuration = (micros() - lastOffTime) / 1000000;

  int tempDiff = (targetTemp - currentTemp);

  if (tempDiff > atTempDiffStartAdjustTimeOnOff)
  {

    if (currentTemp > targetTemp)
      switchOn = false;
    else
      switchOn = true;
  }
  else
  {
    OnOffSec = max(tempDiff * maxOnTempDiffK, minOnOffCycleSec);
  

    if (currentTemp > targetTemp)
      switchOn = false;
    else if ((onDuration > OnOffSec && IsOn) || (offDuration < OnOffSec && !IsOn))
      switchOn = false;
    else if (currentTemp < targetTemp)
      switchOn = true;
  }

  Serial.println(tempDiff);
  Serial.print("on");
  Serial.println(onDuration);
  Serial.print("maxOnOffsec ");
  Serial.println(OnOffSec);
  Serial.print("Is on : ");
  Serial.println(switchOn);

  if (switchOn && !IsOn)
    lastOnTime = micros();

  if (!switchOn && IsOn)
    lastOffTime = micros();

  if (switchOn)
  {
    digitalWrite(2, HIGH);
    IsOn = true;
  }

  else
  {
    digitalWrite(2, LOW);
    IsOn = false;
  }
}

int getCurrentInterval(int minute)
{

  for (int i = 0; i < TempIntervalCnt; i++)
  {

    if (tempIntervals[i].from <= minute && minute <= tempIntervals[i].to)

      return map(minute, tempIntervals[i].from, tempIntervals[i].to, tempIntervals[i].tempFrom, tempIntervals[i].tempTo);
  }
  return -1; //not found
}

void EEPROMWriteInt(int p_address, int p_value)
{
  byte lowByte = ((p_value >> 0) & 0xFF);
  byte highByte = ((p_value >> 8) & 0xFF);

  EEPROM.write(p_address, lowByte);
  EEPROM.write(p_address + 1, highByte);
}

//This function will read a 2 byte integer from the eeprom at the specified address and address + 1
unsigned int EEPROMReadInt(int p_address)
{
  byte lowByte = EEPROM.read(p_address);
  byte highByte = EEPROM.read(p_address + 1);

  return ((lowByte << 0) & 0xFF) + ((highByte << 8) & 0xFF00);
}
