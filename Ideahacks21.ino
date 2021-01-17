// Created by Jonathan Lam for IDEAhacks21

#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include "time.h"

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library. 
// On an arduino UNO:       A4(SDA), A5(SCL)
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
// On an arduino LEONARDO:   2(SDA),  3(SCL), ...
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define button 12
#define LEDR 15
#define LEDG 2
#define LEDB 4
#define Emitter1 32
#define Receiver1 35

#define R_CHANNEL 0
#define G_CHANNEL 1
#define B_CHANNEL 2

#define pwm_frequency 5000
#define pwm_resolution 8

// Function protoypes
void rgbColor(int r, int g, int b, int del); // set rgb to desired color combination for x ms
void printOLED(String text, int x, int y, int sz); // print text onto OLED display
String httpGETRequest(const char* serverName); // access URL in order to communicate with API-spreadsheets
String getTime(); // access current time
String parsingTimes(JSONVar buf); // return latest time that a current event ends
String differenceTime(String latest, String current); // return the difference in time between the end of current event and current time 

// determine whether the button has been pressed / object near IR pair
int buttonState;
int reading;

// Busy / not busy
String currentTime = getTime();
bool busy;

// Global Variables for web server, credentials for wifi
const char* ssid = "FiOS-SY7GJ";
const char* password = "ate2335soup9632sly";
WiFiServer server(80);
unsigned long lastTime = 0;
unsigned long timerDelay = 10000; // how often it updates busy / free n(10 seconds)
String jsonBuffer; // storage for API return data
String latest, elapsed; // keeping track of latest end time of current event, time until it ends

// global variables for accessing time
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = -28800; // PST time zone
const int   daylightOffset_sec = 3600;


void setup() {
  // setting necessary I/O pins
  pinMode(button, INPUT);
  pinMode(Emitter1, OUTPUT);
  pinMode(Receiver1, INPUT);
  ledcAttachPin(LEDR, R_CHANNEL);
  ledcAttachPin(LEDG, G_CHANNEL);
  ledcAttachPin(LEDB, B_CHANNEL);
  ledcSetup(R_CHANNEL, pwm_frequency, pwm_resolution);
  ledcSetup(G_CHANNEL, pwm_frequency, pwm_resolution);
  ledcSetup(B_CHANNEL, pwm_frequency, pwm_resolution);
  Serial.begin(115200);

  digitalWrite(Emitter1, HIGH); // turning on IR emitter indefinitely
  delay(1000);
  
  rgbColor(0, 0, 0, 100); // turning RGB LED off
  
  // setup for web server
  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());

  // setup for accessing time
  //init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // intializing sequence after the wifi has successfully connected
  display.clearDisplay();
  printOLED("Loading...", 5, 25, 2);
  display.display();
  delay(2000);
  display.clearDisplay();
  printOLED("",0,0,1);
  display.display();
  printOLED("Am I Busy?", 5, 25, 2);
  display.display();
  delay(3000);
}

void loop() {
  currentTime = getTime();
  display.clearDisplay(); // clearing the previous content on OLED display
  printOLED("",0,0,1);
  display.display();
  buttonState = digitalRead(button); // read in state of button
  reading = analogRead(Receiver1); // read in value from IR Phototransistor
  if ((millis() - lastTime) > timerDelay)  // ensuring that too many API calls aren't made
  {
    if(WiFi.status()== WL_CONNECTED){
      String serverPath = "https://api.apispreadsheets.com/data/6638/"; // connecting to API-spreadsheets
      
      jsonBuffer = httpGETRequest(serverPath.c_str()); // retrieving information from database
      Serial.println(jsonBuffer);
      JSONVar myObject = JSON.parse(jsonBuffer);
  
      // JSON.typeof(jsonVar) can be used to get the type of the var
      if (JSON.typeof(myObject) == "undefined") {
        Serial.println("Parsing input failed!");
        return;
      }
      Serial.print("JSON object = ");
      Serial.println(myObject);
      latest = parsingTimes(myObject); // returning latest end time of currently onging event
      elapsed = differenceTime(latest, currentTime); // difference between current time and latest end time
      if (elapsed == "")
        busy = false;
      else
        busy = true;
      lastTime = millis(); // update lastTime to current time after calling API
    }
  }
  if (reading > 300) // condition checking if object is near IR Emitter Pair
  {
    if (busy) // true is there is currently an event going on
    {
      printOLED("Busy for", 10, 15, 2.25);
      printOLED("another", 15, 30, 2.25);
      printOLED("~" + elapsed + "hr(s)", 20, 45, 2.25);
      display.display();
      rgbColor(100,0,0,1);
    }
    else
    {
      // if no events are occurring, then the user is currently free
      printOLED("Currently free!", 10, 15, 2.25);
      display.display();
      rgbColor(0,100,0,1);
    }
    delay(5000);
    rgbColor(0,0,0,1);
  }
  else if (buttonState == HIGH)
  {
    // if button is pressed, display current time based on 24 hr clock cycle
    String tm = getTime();
    tm = tm.substring(0,2) + ":" + tm.substring(2,4);
    printOLED(tm, 15, 20, 3);
    display.display();
    rgbColor(100, 100, 0, 1);
    delay(3000);
    rgbColor(0,0,0,1);
  }
  else
  {
    // do nothing if neither the button or IR emitter pair are HIGH, preserve energy from not using OLED & RGB LED
  }
}


void rgbColor(int r, int g, int b, int del)
{
  ledcWrite(R_CHANNEL, r);
  ledcWrite(G_CHANNEL, g);
  ledcWrite(B_CHANNEL, b);
  delay(del);
}

void printOLED(String text, int x, int y, int sz)
{
  display.setTextSize(sz);
  display.setTextColor(WHITE);
  display.setCursor(x,y);
  display.println(text);
}

String httpGETRequest(const char* serverName) {
  HTTPClient http;
    
  // Your IP address with path or Domain name with URL path 
  http.begin(serverName);
  
  // Send HTTP POST request
  int httpResponseCode = http.GET();
  
  String payload = "{}"; 
  
  if (httpResponseCode>0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();

  return payload;
}

String getTime()
{
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return "";
  }
  String h = String(timeinfo.tm_hour);
  String m = String(timeinfo.tm_min);
  // condition where no leading 0 exists, need to format it properly to uniform length
  if(h.length()==1)
  {
    h = "0"+h;
  }
  if(m.length()==1)
  {
    m = "0"+m;
  }
  String t = h+m;
  return t;
}

String parsingTimes(JSONVar buf)
{
  String latest = "";
  for (int i = 0; i < buf["data"].length(); i++)
  {
    // removing quotes as a result of stringify
    String start_time = JSON.stringify(buf["data"][i]["Start Time"]).substring(1);
    String end_time = JSON.stringify(buf["data"][i]["End Time"]).substring(1);
    start_time = start_time.substring(0, start_time.length()-1);
    end_time = end_time.substring(0, end_time.length()-1);
    if (start_time.length() == 4) // missing initial 0, 8:00
      start_time = "0" + start_time;
    if (end_time.length() == 4) // missing intial 0, 2:00
      end_time = "0" + end_time;
    start_time = start_time.substring(0,2) + start_time.substring(3,5);
    end_time = end_time.substring(0,2) + end_time.substring(3,5);
    Serial.println(start_time + " " + end_time);
    if(start_time < currentTime && currentTime < end_time) // event is happening currently
    {
      Serial.println(end_time);
      if (latest=="")
        latest = end_time;
      else
      {
        if (end_time > latest) // if its later than the latest current event, update it
          latest = end_time;  
      }
    }
  }
  return latest;
}

String differenceTime(String latest, String current)
// latest = L1:L2 
// current = C1:C2
// ex) 08:12; L1 = 8; L2 = 12
{
  if (latest == ""){ //condition where there are no events which are currently ongoing
    return "";
  }
  int L1 = latest.substring(0,2).toInt();
  int L2 = latest.substring(2,4).toInt();
  int C1 = current.substring(0,2).toInt();
  int C2 = current.substring(2,4).toInt();
  String D1,D2;
  if(C2 > L2)
  {
    L1--;
    L2+=60;
  }
  int d2 = L1 - C1;
  if (L2 - C2 >= 30)
    d2++;
  // return to the closest hour 
  return String(d2); 
}
