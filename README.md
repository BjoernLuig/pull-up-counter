# pull-up-counter
A device to count pull ups per day, week and month using an ultrasonic ditance sensor. It is based on an ESP32 Dev Module, an ultrasonic sensor and an OLED display enclosed in an 3D printed case.
<img src="https://github.com/BjoernLuig/pull-up-counter/blob/master/photos/far.jpg?raw=true" width=500>

## Functionality
The device connects to wifi on boot to get the currunt time. The pull up counts per day, week and month are saved on the EEPROM to survive a shutdown. Everyday at 4 am the daily counter is reset. At 8pm the display is white until you reach your daily goal of 10 pull ups. Only after that the display can time out. To still see your currunt counts in that case you can move infront of the sensor in a distance between 80 and 30 cm so it is not counted as a pull up. All this parameters can be changed in the first lines of the arduino code on "pull-up-counter.ino".
<img src="https://github.com/BjoernLuig/pull-up-counter/blob/master/photos/close.jpg?raw=true" width=500>

## Build
The stl files for the 3D printed case "upper-case.stl" and "lower-case.stl" can be found in the repo. The diameter of the pull up bar in this case is 65.4 mm. If you want to change it, you can copy my [project from oneshape](https://cad.onshape.com/documents/7eee93e0a96726aa8824ecba/w/20d27a70971ff588a796e390/e/7e2507949053c2282746da28?renderMode=0&uiState=6424a03c8284476ec38abdca) and edit the radius in sketch 3. After printing you need to insert 4 threaded M3 inserts usong a soldering iron. You also need 4 M3 screws. The electronic componants can be fixated using hotglue in the corners.
<img src="https://github.com/BjoernLuig/pull-up-counter/blob/master/photos/prints.png?raw=true" width=500>
<img src="https://github.com/BjoernLuig/pull-up-counter/blob/master/photos/wirering.png?raw=true" width=500>
<img src="https://github.com/BjoernLuig/pull-up-counter/blob/master/photos/inside.jpg?raw=true" width=500>

## Software
before uploading the arduino code you need to cerate a file called "secrets.h" containig your wifi credentials in the following format: 

    #define SSID "YOURSSID"
    #define PASSWORD "YOURPASSWORD"
  
Furthermore you may need to edit

    #define TIME_ZONE "CET-1CEST,M3.5.0/02,M10.5.0/03"
    
in the pull-up-counter.ino to match your [region format](https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv).
