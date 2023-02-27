// A device to count pull ups per day, week and month using an ultrasonic ditance sensor.
// The counts are saved on the EEPROM.
// The currunt time is optained through a time server at every boot.

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <NewPing.h>
#include <EEPROM.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "secrets.h"

// distance settings in cm
#define MAX_DISTANCE 80     // maximum distance to detect anything (max 250)
#define PULL_UP_DISTANCE 20 // distance under which a pull up is detected

// timing settings in milliseconds
#define LOOP_DELAY 100            // delay for main loop in milliseconds (optimal 100)
#define DATA_SAVE_INTERVALL 60000 // save intervall of data to EEPROM (100.000 value changes per lifetime)
#define RESET_TIMEOUT 10000       // reset data on EEPROM if in PULL_UP_DISTANCE for this time
#define DISPLAY_TIMEOUT 30000     // timeout for display (only if the daily goal is achived)

// daily routine settings
#define MIN_PULL_UPS_PER_DAY 10 // your daily goal which keeps the display on until you achive it
#define MAX_WAKE_UP_HOUR 8      // when the display kann show your daily goal in the morning

// display
Adafruit_SSD1306 display(128, 64, &Wire, -1);

// distance sensor
#define TRIGGER_PIN 27
#define ECHO_PIN 14
NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE);
int distance;

// pull up counter
boolean pullUpDetected = false;
int countThisDay;
int countThisWeek;
int countThisMonth;

// timing
long lastDetection = 0;
long lastPullUpDetection = 0;
long lastDataSave = 0;

// datetime
#define TIME_OFFSET 3600 // time offset to standard time (in german: winter 3600 second = 1 hour, summer 7200 = 2 hours)
#define TIME_SERVER "pool.ntp.org"
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, TIME_SERVER);
char timeString[50];
int curruntWeekDay;
int curruntMonth;
int lastWeekDay;
int lastMonth;

void setup()
{

  // serial
  Serial.begin(115200);
  Serial.println("setup");

  // display
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("setup");
  display.display();

  // EEPROM
  EEPROM.begin(6); // 3 2 byte integers
  EEPROM.get(0, countThisDay);
  EEPROM.get(2, countThisWeek);
  EEPROM.get(4, countThisMonth);
  EEPROM.get(6, curruntWeekDay);
  EEPROM.get(8, curruntMonth);

  // connect wifi
  WiFi.begin(SSID, PASSWORD);
  Serial.print("connecting to wifi");
  display.print("connecting to wifi");
  display.display();
  for (int i = 0; i < 60; i++)
  { // try for 60 seconds
    if (WiFi.status() != WL_CONNECTED)
    {
      Serial.print(".");
      display.print(".");
      display.display();
      delay(1000);
    }
    else
    {
      break;
    }
  }
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println();
    Serial.println("no wifi after 60 seconds");
    Serial.println("please restart");
    display.println();
    display.println("no wifi after 60 seconds");
    display.println("please restart");
    display.display();
    while (true)
      ;
  }
  else
  {
    Serial.println();
    Serial.println("wifi connected");
    display.println();
    display.println("wifi connected");
    display.display();

    // connect to npt server
    timeClient.begin();
    timeClient.setTimeOffset(3600);
    Serial.print("waiting for ntp");
    display.print("waiting for ntp");
    display.display();
    while (!timeClient.update())
    {
      timeClient.forceUpdate();
      Serial.print(".");
      display.print(".");
      display.display();
      delay(500);
    }
    Serial.println("");
    Serial.println("ntp time synced");
    display.println("");
    display.println("ntp time synced");
    display.display();

    // sync npt time with internal rtc
    struct tm timeinfo;
    time_t now = timeClient.getEpochTime();
    gmtime_r(&now, &timeinfo);
    struct timeval tv = {.tv_sec = mktime(&timeinfo)};
    settimeofday(&tv, NULL);
    Serial.print("new rtc time: ");
    Serial.println(timeClient.getFormattedTime());
    display.print("new rtc time: ");
    display.println(timeClient.getFormattedTime());
    display.display();

    // disconnect wifi
    WiFi.disconnect(true);
    Serial.println("wifi disconnected");
    display.println("wifi disconnected");
    display.display();
  }
  Serial.println();

  // delay to read display
  delay(3000);
}

void loop()
{

  // datetime
  struct tm timeInfo;
  if (!getLocalTime(&timeInfo))
  {
    Serial.println("failed to get time");
    Serial.println("please restart");
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println("failed to get time");
    display.println("please restart");
    display.display();
    return;
  }
  strftime(timeString, sizeof(timeString), "%d.%m %H:%M", &timeInfo);
  Serial.print("time: ");
  Serial.println(timeString);
  lastWeekDay = curruntWeekDay;
  lastMonth = curruntMonth;
  curruntWeekDay = timeInfo.tm_wday; // sunday = 0
  curruntMonth = timeInfo.tm_mon;    // januar = 0
  EEPROM.put(6, curruntWeekDay);
  EEPROM.put(8, curruntMonth);

  // reset counts on new day, week or month
  if (curruntWeekDay != lastWeekDay)
  {
    countThisDay = 0;
    Serial.println("new day");
  }
  if (curruntWeekDay == 1 && lastWeekDay == 0)
  { // monday after sunday
    countThisWeek = 0;
    Serial.println("new week");
  }
  if (curruntMonth != lastMonth)
  {
    countThisMonth = 0;
    Serial.println("new month");
  }

  // detect distance
  distance = sonar.ping_cm();
  if (distance != 0 && distance < MAX_DISTANCE)
  {
    lastDetection = millis();
    Serial.print("detected distance: ");
    Serial.print(distance);
    Serial.println(" cm");
  }

  // detect pull up
  if (distance != 0 && distance < PULL_UP_DISTANCE)
  {
    if (!pullUpDetected)
    {
      pullUpDetected = true;
      lastPullUpDetection = millis();
      countThisDay++;
      countThisWeek++;
      Serial.println("pull up detected");
    }
  }
  else
  {
    pullUpDetected = false;
  }

  // reset counts on active reset
  if (pullUpDetected && millis() - lastPullUpDetection > RESET_TIMEOUT)
  {
    countThisDay = 0;
    countThisWeek = 0;
    countThisMonth = 0;
    Serial.println("reset counts on active reset");
  }

  // save counts to EEPROM
  if (millis() - lastDataSave > DATA_SAVE_INTERVALL)
  {
    lastDataSave = millis();
    EEPROM.put(0, countThisDay);
    EEPROM.put(2, countThisWeek);
    EEPROM.put(4, countThisMonth);
    Serial.println("saved counts to EEPROM");
  }

  // serial
  Serial.print("distance: ");
  Serial.println(distance);
  Serial.print("countThisDay: ");
  Serial.println(countThisDay);
  Serial.print("countThisWeek: ");
  Serial.println(countThisWeek);
  Serial.print("countThisMonth: ");
  Serial.println(countThisMonth);
  Serial.println();

  // display
  display.clearDisplay();
  if ((millis() - lastDetection < DISPLAY_TIMEOUT) || (countThisDay < MIN_PULL_UPS_PER_DAY && timeInfo.tm_hour >= MAX_WAKE_UP_HOUR))
  {
    display.setTextSize(1);
    display.setCursor(30, 0);
    display.println(timeString);
    display.setTextSize(2);
    display.setCursor(0, 16);
    display.print(countThisDay);
    display.setCursor(24, 16);
    display.println("pull ups");
    display.setTextSize(1);
    display.setCursor(0, 40);
    display.print("this week: ");
    display.println(countThisWeek);
    display.print("this month: ");
    display.println(countThisMonth);
    display.print("distance: ");
    display.print(distance);
    display.print(" cm");
    display.display();
  }

  // loop delay
  delay(LOOP_DELAY);
}