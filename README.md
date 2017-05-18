# furnace-temperature-controller

## Hardware

[![video](http://img.youtube.com/vi/bsTQhCWisBk/0.jpg)](https://youtu.be/bsTQhCWisBk)

![photo](/furnace.jpg)



###Motivation 

For jewelry casting one need to control the furnce temperature as in example below :
![photo](/casting.png)

This is acieved by having tempertature intervals.


### Hadrware 
TODO

###Settings

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
