/////////////////////////////////////////////////////////////////////////
// We build a water storage using 9 IPC Containers that are connected  //
// to each otehr and in which we collect rainwater.                    //
// The containers are under the ground and we built a shed/poolhouse   //
// on top of it.                                                       //
// We have a hydrophore pump to use the water in the garden and we     // 
// connected a valve to swhich between the pump and normal tapwater.   //
// This program is made for an Arduino to control the water system:    //
// - we use a pulse sensor to measure the water level in the containers//
// - to get a precice measuement, a thermometer measures the air       //
//   temprature in the containers.                                     //
// - there is a oled disply to display the waterlevel and other info   //
// - there is a (two phase) electronic switch valve, switching between //
//   pump and tap water.                                               //
// - there are two ralais to set the valve to one or the other         //
//   direction                                                         //
//                                                                     //
// The process works as follows:                                       //
// - periodically the arduino measures the water level                 //
// - if the level is below a certain treshold, the system will switch  //
//   to tapwater                                                       //
// - if the level is above a certain level, the system will switch to  //
//   pumpwater                                                         //
/////////////////////////////////////////////////////////////////////////
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1331.h>
#include <SPI.h>
//#include <Fonts/FreeSerifItalic24pt7b.h>
#include <Fonts/FreeSerifItalic18pt7b.h>
//#include <Fonts/FreeSerifItalic12pt7b.h>
#include <Fonts/FreeSerifItalic9pt7b.h>
#include <OneWire.h> 
#include <DallasTemperature.h>
#include <ThreeWire.h>  
#include <RtcDS1302.h>
#include <EEPROM.h>

#define VATEN_VOL 101.1 // Measured Distance when the containers are filled 
#define VATEN_LEEG 201.0 // Measured Distance when the containers are empty

/////////////////////
// Pin Definitions //
/////////////////////
// Display Pins
// You can use any (4 or) 5 pins
#define sclk 13
#define mosi 11
#define cs   10
#define rst  9
#define dc   8

// Ultrasone Sensor Pins
#define echoPin 2 // attach pin D2 Arduino to pin Echo of HC-SR04 (tx)
#define trigPin 3 //attach pin D3 Arduino to pin Trig of HC-SR04 (rx)

// Data wire is plugged into pin 4 on the Arduino (DS18S20  ) temporary 1 for the test
#define ONE_WIRE_BUS 4 

// Pins for the realtime clock
#define RtcDS1302_CLK_SCLK 5
#define RtcDS1302_DAT_IO   6
#define RtcDS1302_RST_CE   7

// Pins to be used by the relais (to control the valves in the future)
#define RELAY_ON LOW
#define RELAY_OFF HIGH
#define RELAY_VALVE_TO_PUMP 15
#define RELAY_VALVE_TO_TAP 14
byte currentValve = 0;

// Color definitions
#define BLACK           0x0000
#define BLUE            0x001F
#define RED             0xF800
#define GREEN           0x07E0
#define CYAN            0x07FF
#define MAGENTA         0xF81F
#define YELLOW          0xFFE0
#define WHITE           0xFFFF
#define LIGHTBLUE       0x9FFF
#define DARKBLUE        0x006D

//History of measurements
const int historySize = 96;
// const unsigned long OneDayInMilliSeconds = 21600000; // 4 keer per dag
const unsigned long OneDayInMilliSeconds = 86400000;
const int DelayInterval = 3500;

/*********************************************************************/
// Setup Time module
ThreeWire myWire(RtcDS1302_DAT_IO ,RtcDS1302_CLK_SCLK,RtcDS1302_RST_CE); // IO, SCLK, CE
RtcDS1302<ThreeWire> Rtc(myWire);
RtcDateTime compiletime = RtcDateTime(__DATE__, __TIME__); 


/********************************************************************/
// Setup a oneWire instance to communicate with any OneWire devices  
// (not just Maxim/Dallas temperature ICs) 
OneWire oneWire(ONE_WIRE_BUS); 
/********************************************************************/
// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);
/********************************************************************/ 

// Display driver:
// Option 1: use any pins but a little slower
//Adafruit_SSD1331 display = Adafruit_SSD1331(cs, dc, mosi, sclk, rst);

// Option 2: must use the hardware SPI pins
// (for UNO thats sclk = 13 and sid = 11) and pin 10 must be
// an output. This is much faster - also required if you want
// to use the microSD card (see the image drawing example)
Adafruit_SSD1331 display = Adafruit_SSD1331(&SPI, cs, dc, rst);

void setup() {
  Serial.begin(9600);
  Serial.println("Starting up");
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an OUTPUT
  pinMode(echoPin, INPUT); // Sets the echoPin as an INPUT

  pinMode(RELAY_VALVE_TO_PUMP, OUTPUT);
  pinMode(RELAY_VALVE_TO_TAP, OUTPUT);
  digitalWrite(RELAY_VALVE_TO_PUMP, RELAY_OFF);
  digitalWrite(RELAY_VALVE_TO_TAP, RELAY_OFF);

  Serial.println("... display");
  
  display.begin();
  display.fillScreen(BLACK);
  displayInformation("Welcome", "Starting", LIGHTBLUE, BLACK);

  Serial.println("... sensors");
  
  sensors.begin(); 
  Rtc.Begin();
  CheckTime();
  delay(1000);
}

void loop() {
  byte previous[historySize];
  //for(int i=0; i<=historySize + 2 ; i++) EEPROM.update(i,0); // reset stored values
  for(int i=0; i<historySize ; i++) previous[i] = EEPROM.read(i);
  bool roundRobin = EEPROM.read(historySize+2) == 1;
  byte currentDay = EEPROM.read(historySize+1);
  unsigned long lasttime = millis();
  while (true)
  {
    RtcDateTime now = Rtc.GetDateTime();
    displayInformation("Date:", getDate(now),BLUE, BLACK);
    delay(DelayInterval);
    displayInformation("Time:", getTime(now),BLUE, BLACK);
    delay(DelayInterval);
    float distance = measureDistance();
    displayInformation("Distance in centimeter:", String(distance), GREEN, BLACK);
    delay(DelayInterval);
    displayInformation("Volume in liters:", String(getVolumeInLiters(distance)), LIGHTBLUE, BLACK);
    delay(DelayInterval);
    byte currentPercentage = previous[currentDay] = (byte)(int)(getVolumePercentage(distance) * 100 + 0.5);
    displayInformation("Waterlevel:", String(currentPercentage) + '%', LIGHTBLUE, BLACK);
    delay(DelayInterval);
    char buf[10];
    dtostrf(getCurrentTemperature(), 4, 1, buf);
    displayInformation("Temperature:", String(buf)+ "C", YELLOW, BLACK);
    delay(DelayInterval);
    displayGraph("Waterlevel", currentDay, roundRobin, previous, historySize);
    delay(DelayInterval); 
    setValves(currentPercentage);
    displayInformation("Watersource:", currentValve == RELAY_VALVE_TO_PUMP ? "Containers" : currentValve == RELAY_VALVE_TO_TAP ? "Tapwater" : "Unknown", LIGHTBLUE, BLACK);
    delay(DelayInterval); 
   
    unsigned long currenttime = millis();
    if ((lasttime > currenttime) || (currenttime - lasttime > OneDayInMilliSeconds))
    {
      // a new day
      EEPROM.update(currentDay, previous[currentDay]);
      currentDay++;
      if (currentDay == historySize)
      {
        roundRobin = true;
        EEPROM.update(historySize+2,1);
        currentDay = 0;
      }
      EEPROM.update(historySize+1,currentDay);
      lasttime = currenttime; 
    }

  }
}
void CheckTime()
{
   if (!Rtc.IsDateTimeValid()) Rtc.SetDateTime(compiletime);
   if (Rtc.GetIsWriteProtected())  Rtc.SetIsWriteProtected(false);
   if (!Rtc.GetIsRunning()) Rtc.SetIsRunning(true);
   if (Rtc.GetDateTime() < compiletime) Rtc.SetDateTime(compiletime);
}

void setValves(byte waterlevelInPercentage)
{
  if (waterlevelInPercentage < 10 || waterlevelInPercentage > 150) 
  {
      SwitchTo(RELAY_VALVE_TO_TAP, "TAP"); 
  }
  if (waterlevelInPercentage > 13 && waterlevelInPercentage < 150) // 150 to protect from sensor failure
  {
     SwitchTo(RELAY_VALVE_TO_PUMP, "PUMP");
  }
}

void SwitchTo(int valveTo, String valveName)
{
  if (currentValve != valveTo)
  {
      displayInformation("SwitchTo:", valveName + " (" + valveTo +")",BLUE, BLACK);
      digitalWrite(valveTo, RELAY_ON);
      delay(20000); // wait 20 seconds to switch
      digitalWrite(valveTo, RELAY_OFF);
      currentValve = valveTo; 
  }
}

#define countof(a) (sizeof(a) / sizeof(a[0]))

String getTime(const RtcDateTime& dt)
{
  char timestring[10];
   snprintf_P(timestring, 
            countof(timestring),
            PSTR("%02u:%02u"),
            dt.Hour(),
            dt.Minute());
  return (String)timestring;            
}


String getDate(const RtcDateTime& dt)
{
  char timestring[12];
   snprintf_P(timestring, 
            countof(timestring),
            PSTR("%02u-%02u-%04u"),
            dt.Day(),
            dt.Month(),
            dt.Year() );
  return (String)timestring;            
}

int getVolumeInLiters(float distance)
{
    return (int)(getVolumePercentage(distance) * 9000);  
}

float getVolumePercentage(float distance)
{
  return (VATEN_LEEG - distance) / (VATEN_LEEG - VATEN_VOL);
}

void setTitleAndClearBackground(String title, int textcolor, int backgroundcolor)
{
  Serial.println(title);
  display.fillScreen(backgroundcolor);
   display.setTextColor(textcolor);
  display.setCursor(0,0);
  display.print(title);
}

void displayInformation(String title, String Info, int textcolor, int backgroundcolor)
{
  setTitleAndClearBackground(title, textcolor, backgroundcolor);
  displayCentreText(Info, textcolor, backgroundcolor);
}

void displayCentreText(String text, int textcolor, int backgroundcolor)
{
  Serial.println(text);
  display.setCursor(0,45);
  display.setTextColor(textcolor);
  //display.setTextSize(3);
  display.fillRect(0, 20 , 127, 33, backgroundcolor);

  if (text.length() < 6)
    display.setFont(&FreeSerifItalic18pt7b);
  //else if (text.length() < 10)
  //  display.setFont(&FreeSerifItalic12pt7b);
  else if (text.length() < 14)
    display.setFont(&FreeSerifItalic9pt7b);
  else display.setFont();  
  display.print(text);
  display.setFont();  
}

void displayGraph(String caption, int currentDay, bool isRoundrobin, byte previous[], int sizeofarray)
{
   setTitleAndClearBackground(caption, BLUE, BLACK);
  display.drawFastHLine(0, 63, sizeofarray, RED);
  display.drawFastHLine(0, 13, sizeofarray, LIGHTBLUE);
  int x =0;
  if (isRoundrobin)
  {
     x = drawgraphpart(currentDay + 1, sizeofarray, previous, x);      
  }
  x = drawgraphpart(0, currentDay + 1, previous, x);       
}

int drawgraphpart( int startinarray, int endinarray, byte previous[], int x)
{
   for (int i = startinarray; i < endinarray; i++)
   {
      int linesize = previous[i]/2;
      if (linesize>50) linesize = 50; 
      int color = (linesize > 30) ? BLUE : 
                  ((linesize > 20) ? GREEN :
                  ((linesize > 10) ? YELLOW : RED
                  )); 
      display.drawFastVLine(x, 63 - linesize, linesize, color);
      x++;
   }    
   return x;  
}

float getDistance()
{
  // Clears the trigPin condition
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  // Sets the trigPin HIGH (ACTIVE) for 10 microseconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  // Reads the echoPin, returns the sound wave travel time in microseconds
  int duration = pulseIn(echoPin, HIGH);
  
  return timeToDistance(duration);
}

float timeToDistance(int duration)
{
  // Calculating the distance (simple)
  // float distance = duration * 0.034 / 2; // Speed of sound wave divided by 2 (go and back)

  float c = getCurrentTemperature();
  if (c < -30 || c > 80) c = 20.0; // The thermometer reports -127C when it's not found or 85C when there is an error. In that case we'll calculate with 20C

  // Using practical airspeed formula from https://en.wikipedia.org/wiki/Speed_of_sound
   float distance = 0.00005 * duration * 20.05 * sqrt(c + 273.15);
   return distance;
}

float getCurrentTemperature()
{
  sensors.requestTemperatures();
  float c = sensors.getTempCByIndex(0);
  return c;
}

float measureDistance()
{
  float totalDistance = 0;
  int measuretimes = 0;
  for (int i=0;i<10;i++)
  {
    float distance = getDistance();
    if ((distance > 0.1) && (distance < 500))
    {
      totalDistance += distance;
      measuretimes++;   
    } 
  }
  return measuretimes > 0 ? totalDistance/measuretimes : 0; 
}
