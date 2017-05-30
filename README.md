# furnace-temperature-controller

## Hardware

- Arduino Nano
- Solid State Relay 40A / 3-32VDC 24-380VAC DC SSR 
- Stainless Steel High Temperature -100 To 1250 Degree Thermocouple K type 100mm Probe
- 1602 LCD screen (blue screen) with backlighting
- Adafruit Thermocouple Amplifier MAX31855 Breakout Board 
- 2AMP 5v DC, Box


[![video](http://img.youtube.com/vi/bsTQhCWisBk/0.jpg)](https://youtu.be/bsTQhCWisBk)


### Motivation 

For jewelry casting one need to control the furnce temperature as in example below :
![photo](/casting.png)

This is acieved by having tempertature intervals.




### Settings

Setting temperature intervals

````
 *    TempInterval(from  minute, to  minute, from temp, to temp)
 *    from minute 0 to 7 raise  temperature from 200 to 300
 *     TempInterval(0, 7, 200, 300),
 
TempInterval tempIntervals[TempIntervalCnt] =
{
        TempInterval(0, 7, 50, 150),
        TempInterval(7, 15, 150, 200),
        TempInterval(15, 30, 200, 200),
        TempInterval(30, 45, 200, 300),
        TempInterval(45, 60, 300, 150),
        TempInterval(60, 90, 150, 150)
};

````

Set PID 
PID myPID(&Input, &Output, &Setpoint,2,1,1, DIRECT);
Info about PID can be found here https://playground.arduino.cc/Code/PIDLibrary



### TODOs
- Beep on finish
- Multiple programms
