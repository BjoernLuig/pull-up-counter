// A device to count pull ups per day, week and month using an ultrasonic ditance sensor.
// The counts are saved on the EEPROM.
// The currunt time is optained through a time server at every boot.

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <NewPing.h>
#include <WiFi.h>
#include <time.h>
#include <Preferences.h>
#include "secrets.h" // file where SSID and PASSWORD for your local network are defined

// daily routine settings
#define PULL_UP_GOAL 10 // your daily goal which keeps the display on until you achive it
#define NEXT_DAY_HOUR 4 // when the daily counter should reset for the next da (optimal at 04:00)
#define WAKE_UP_HOUR 8  // when the display can show your daily goal in the morning (08:00)

// distance settings in cm
#define MAX_DISTANCE 80     // maximum distance to detect anything (max 250)
#define PULL_UP_DISTANCE 35 // distance under which a pull up is detected

// datetime settings
#define TIME_ZONE "CET-1CEST,M3.5.0/02,M10.5.0/03" // from https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
#define TIME_SERVER "pool.ntp.org"                 // address of timeserver
struct tm timeInfo;
char timeString[50];
int curruntHour;
int curruntWeekDay;
int curruntMonth;
int lastHour;
int lastWeekDay;
int lastMonth;

// loop timing settings in milliseconds
#define LOOP_DELAY 100               // delay for main loop in milliseconds (optimal 100)
#define PULL_UP_DELAY 1000           // min time between to pull ups to count (optimal 1000)
#define DATA_SAVE_INTERVALL 10000    // save intervall of data to EEPROM
#define RESET_THIS_DAY_TIMEOUT 10000 // manuel reset day counter if in PULL_UP_DISTANCE for this time
#define RESET_ALL_TIMEOUT 20000      // manuel reset all data on EEPROM if in PULL_UP_DISTANCE for this time
#define DISPLAY_TIMEOUT 10000        // timeout for display (only if the daily goal is achived)

// distance sensor
#define TRIGGER_PIN 27
#define ECHO_PIN 14
NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE);
int distance;

// display
Adafruit_SSD1306 display(128, 64, &Wire, -1);

// pull up counter
boolean pullUpDetected = false;
int countThisDay;
int countThisWeek;
int countThisMonth;

// timing
long lastDetection = 0;
long lastPullUpDetection = 0;
long lastDataSave = 0;

// preferences
Preferences preferences;

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

  // preferences
  preferences.begin("data", false);
  curruntHour = preferences.getInt("curruntHour", 0);
  curruntWeekDay = preferences.getInt("curruntWeekDay", 0);
  curruntMonth = preferences.getInt("curruntMonth", 0);
  countThisDay = preferences.getInt("countThisDay", 0);
  countThisWeek = preferences.getInt("countThisWeek", 0);
  countThisMonth = preferences.getInt("countThisMonth", 0);
  preferences.end();

  // connect wifi
  WiFi.begin(SSID, PASSWORD);
  Serial.print("connecting to wifi");
  display.print("connecting to wifi");
  display.display();
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    display.print(".");
    display.display();
    delay(1000);
  }
  Serial.println();
  Serial.println("wifi connected");
  display.println();
  display.println("wifi connected");
  display.display();
  delay(2000); // wail for readability

  // connect to npt server and set timezone
  configTime(0, 0, TIME_SERVER); // 0, 0 because we will use TZ in the next line
  setenv("TZ", TIME_ZONE, 1);    // Set environment variable with your time zone
  tzset();
  while (!getLocalTime(&timeInfo))
  {
    Serial.println("failed to get time info");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("failed to get time info");
    display.display();
    delay(500);
    display.clearDisplay();
    display.display();
    delay(500);
  }
  strftime(timeString, sizeof(timeString), "%d.%m %H:%M", &timeInfo);
  Serial.print("new time: ");
  Serial.println(timeString);
  display.print("new time: ");
  display.println(timeString);
  display.display();

  // disconnect wifi
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  Serial.println("wifi disconnected");
  display.println("wifi disconnected");
  display.display();
  delay(2000); // wail for readability
}

void loop()
{

  // datetime
  while (!getLocalTime(&timeInfo))
  {
    Serial.println("failed to get time info");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("failed to get time info");
    display.display();
    delay(500);
    display.clearDisplay();
    display.display();
    delay(500);
  }
  strftime(timeString, sizeof(timeString), "%d.%m %H:%M", &timeInfo);
  lastHour = curruntHour;
  lastWeekDay = curruntWeekDay;
  lastMonth = curruntMonth;
  curruntHour = timeInfo.tm_hour;
  curruntWeekDay = timeInfo.tm_wday; // sunday = 0, monday =1, ...
  curruntMonth = timeInfo.tm_mon;    // januar = 0

  // reset counts on new day, week or month
  if (curruntHour == NEXT_DAY_HOUR && curruntHour != lastHour)
  {
    countThisDay = 0;
    Serial.println("new day");
  }
  if (curruntWeekDay == 1 && curruntWeekDay != lastWeekDay)
  {
    countThisWeek = 0;
    Serial.println("new week");
  }
  if (lastMonth != curruntMonth)
  {
    countThisMonth = 0;
    Serial.println("new month");
  }

  // detect distance
  distance = sonar.ping_cm();
  if (distance != 0)
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
      countThisMonth++;
      Serial.print("detected pull up at ");
      Serial.println(timeString);
      Serial.print("countThisDay: ");
      Serial.println(countThisDay);
      Serial.print("countThisWeek: ");
      Serial.println(countThisWeek);
      Serial.print("countThisMonth: ");
      Serial.println(countThisMonth);
    }
  }
  else
  {
    if (millis() - lastPullUpDetection > PULL_UP_DELAY)
    {
      pullUpDetected = false;
    }
  }

  // reset countsThisDay on active reset
  if (pullUpDetected && millis() - lastPullUpDetection > RESET_THIS_DAY_TIMEOUT)
  {
    countThisWeek -= countThisDay;
    countThisMonth -= countThisDay;
    countThisDay = 0;
    Serial.println("reset countThisDay on active reset");
  }

  // reset all counts on active reset
  if (pullUpDetected && millis() - lastPullUpDetection > RESET_ALL_TIMEOUT)
  {
    countThisDay = 0;
    countThisWeek = 0;
    countThisMonth = 0;
    Serial.println("reset all counts on active reset");
  }

  // save counts and time infos using preferences
  if (millis() - lastDataSave > DATA_SAVE_INTERVALL)
  {
    lastDataSave = millis();
    preferences.begin("data", false);
    preferences.putInt("curruntHour", curruntHour);
    preferences.putInt("curruntWeekDay", curruntWeekDay);
    preferences.putInt("curruntMonth", curruntMonth);
    preferences.putInt("countThisDay", countThisDay);
    preferences.putInt("countThisWeek", countThisWeek);
    preferences.putInt("countThisMonth", countThisMonth);
    preferences.end();
    Serial.print("saved counts and time infos");
  }

  // display
  bool timeout = millis() - lastDetection > DISPLAY_TIMEOUT;
  bool earlyMorning = timeInfo.tm_hour <= WAKE_UP_HOUR && timeInfo.tm_hour >= NEXT_DAY_HOUR;
  bool pullUpGoal = countThisDay >= PULL_UP_GOAL;
  display.clearDisplay();
  if (!(timeout && (pullUpGoal || earlyMorning)))
  {
    if (!pullUpGoal)
    {
      display.invertDisplay(true);
    }
    else
    {
      display.invertDisplay(false);
    }
    display.setTextSize(1);
    display.setCursor(32, 4);
    display.println(timeString);
    display.setTextSize(3);
    display.setCursor(48, 17);
    display.print(countThisDay);
    display.setTextSize(1);
    display.print(" /");
    display.print(PULL_UP_GOAL);
    display.setCursor(4, 42);
    display.print("Week: ");
    display.print(countThisWeek);
    display.setCursor(64, 42);
    display.print("Month: ");
    display.print(countThisMonth);
    display.setCursor(4, 52);
    display.print("distance: ");
    display.print(distance);
    display.print(" cm");
  }
  display.display();

  // loop delay and empty Serial.println()
  delay(LOOP_DELAY);
}
