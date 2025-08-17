/*
TODO
1) in case any sensor malfunctions do something (what?)
2) Refactor sensor reading (same code repeating all the time)
3) Teplota vody nefunguje kdyz prekroci cca 64 stupnu, pak dojde do minusu a znovu roste (preteceni nekde?)
*/

/* Smart control of fireplace stove heating and electric heater in one house. 
 Main features are:
  1) Safety - should the water from fireplace be too hot it turns on the pump to cool it down and if it reaches critical threshold an alarm turns on
  2) Thermostat - turns heater on / off as needed to combine the power with fireplace stove and raeach preset temperature.
      Thermostat uses two different sensors for two different scenarios
    a) Winter / antifreeze - sensor 2 that is placed close to plumbing is used to maintain low temperature but avoid the water to freeze (5 degrees)
    b) Normal / Summer - sensor 1 placed in the main room is used to maintaing comfortable temperature (e.g. 22 degrees)
  
  3) Other 
      - the pump as well as main heater are turned on / off with preset delays to avoid their damage in case of frequent switches.
      - the entire system can easily be configured and status checked on 2x16 display
      - the electric heater does not turn on if the stove output water temperature is higher then the preset output temperature of the electric heater.
      - some other small things.
  Language: menu items are in czech. 
*/


#include <Arduino.h>
#include <LiquidCrystal.h>
//#include <OneWire.h> // zakomentovano abych na pinech A4,A5 nemel I2C SDA a SCL protokol a mohl to pouzit pro relatka
#include <DallasTemperature.h>
#include <DHT.h>
#include <EEPROM.h>
#include <avr/pgmspace.h> 
#include "Menu.h"
#include "MySerial.h"

#define VERSION 6 //  increasing the version number will force the replacement of EEPROM content by the defaults. Do it when changing the configuration set.
#define VPOSITION 512 // position in EEPROM to store version number. Must be higher then SETTINGSMENUSIZE 
//#define SENSORDEBUG // uncomment for debugging without the use of sensors. The values are then set in main menu using the keyboard
//#define LOGGING // turn on logging messages on Serial 

#ifdef LOGGING
  #pragma message("Logging to Serial is ENABLED.")
  #define LOG(x)  Serial.println(F(x))
#else 
  #define LOG(x) 
#endif

#ifdef SENSORDEBUG
  #pragma message "Debugging without sensors enabled!"
#endif


enum menu_type {
  MAIN_MENU,
  SETTINGS_MENU
};

enum MainMenuItem {
  ACTROOM1TEMP = 0,   // Measured temperature in Room 1
  ACTROOM1HUM,        // Measured humidity in Room 1
  ACTROOM2TEMP,       // Measured temperature in Room 2
  ACTROOM2HUM,        // Measured humidity in Room 2
  ACTWATEROUTTEMP,    // Measured temperature of water leaving the fireplace stove 
  ACTWATERINTEMP,     // Measured temperature of water coming back to the electrical heater
  ACTPUMPON,          // True if the pump is running
  ACTELHEATERON,      // True if the electric heater is running
  SETTINGS,           
  MAINMENUSIZE
};

const PROGMEM char ENTRY_1[] = "Patro Teplota   ";
const PROGMEM char ENTRY_2[] = "Patro Vlhkost   ";
const PROGMEM char ENTRY_3[] = "Prizemi Teplota ";
const PROGMEM char ENTRY_4[] = "Prizemi Vlhkost ";
const PROGMEM char ENTRY_5[] = "T vystupni vody ";
const PROGMEM char ENTRY_6[] = "T vstupni  vody ";
const PROGMEM char ENTRY_7[] = "Cerpadlo        ";
const PROGMEM char ENTRY_8[] = "Kotel           ";
const PROGMEM char ENTRY_9[] = "Nastaveni -> Sel";

const PROGMEM char *const MMENTRIES_TABLE[] = {ENTRY_1, ENTRY_2, ENTRY_3, ENTRY_4, ENTRY_5, ENTRY_6, ENTRY_7, ENTRY_8, ENTRY_9};

item_t MAINMENU[] = {
  {"", 0, 50, 10000, 0, " \xDF""C"},
  {"", 0, 0, 0, 0, " %"},
  {"", 0, 50, 10000, 0, " \xDF""C"},
  {"", 0, 0, 0, 0, " %"},
  {"", 0, 50, 10000, 0, " \xDF""C"},
  {"", 0, 50, 10000, 0, " \xDF""C"},
  {"", AUTO, 0, 0, 0, ""},
  {"", OFF, 0, 0, 0, ""},
  {"", BLANK, 0, 0, 0, ""}
};

enum SettingMenuItem {
  SETROOM1TEMP = 0,     // The preset temperature for Room 1
  SETROOM2TEMP,         // The preset temperature for Room 2
  SETANTIFREEZE,        // Switch from winter to summer operation. If True, Room 2 temperature is used for thermostat
  SETMAXWATERTEMP,      // Maximum allowed water temperature. If exceeded, the alarm turns on.
  SETELHEATER,          // Mode of operation for heater, ON, OFF, AUTO - thermostat
  SETPUMP,              // Mode of operation for pumo, ON, OFF, AUTO - turns on if water temp is above preset value
  SETHYSTER,            // Thermostat hysterezion
  SETMAXWATEROUTHEATER, //  Max output temperature when the electric heater turns off and no longer contributes to the heating. 
  BACK,
  SETTINGSMENUSIZE
};

#define MINTEMP 0

const PROGMEM char SENTRY_1[] = "Teplota Patro   ";
const PROGMEM char SENTRY_2[] = "Teplota Prizemi ";
const PROGMEM char SENTRY_3[] = "Zimni Provoz    ";
const PROGMEM char SENTRY_4[] = "Max Teplota vody";
const PROGMEM char SENTRY_5[] = "Provoz Kotle    ";
const PROGMEM char SENTRY_6[] = "Provoz Cerpadla ";
const PROGMEM char SENTRY_7[] = "Hystereze       ";
const PROGMEM char SENTRY_8[] = "T Vody z Kotle  ";
const PROGMEM char SENTRY_9[] = "Zpet <- Sel     ";

const PROGMEM char *const SMENTRIES_TABLE[] = {SENTRY_1, SENTRY_2, SENTRY_3, SENTRY_4, SENTRY_5, SENTRY_6, SENTRY_7, SENTRY_8, SENTRY_9};

item_t SETTINGSMENU[] =  {
  {"", DEFAULT_ROOM1_TEMP, TEMPSTEP, 10000, MINTEMP, " \xDF""C"},
  {"", DEFAULT_ROOM2_TEMP, TEMPSTEP, 1000, MINTEMP, " \xDF""C"},
  {"", ON, 1, ON, OFF, ""},
  {"", DEFAULT_MAX_WATER_TEMP, TEMPSTEP, 9000, MINTEMP, " \xDF""C"},
  {"", DEFAULT_HEATER, 1, AUTO, OFF, ""},
  {"", DEFAULT_PUMP, 1, AUTO, OFF, ""},
  {"", DEFAULT_HYSTEREZE, TEMPSTEP, 300, MINTEMP, " \xDF""C"},
  {"", DEFAULT_HEATEROUTWATER, TEMPSTEP, 80000, 40000, " \xDF""C"},  
  {"", BLANK, 0, 0, 0, ""}
};

//LCD pin to Arduino
const int pin_RS = 8;
const int pin_EN = 9;
const int pin_d4 = 4;
const int pin_d5 = 5;
const int pin_d6 = 6;
const int pin_d7 = 7;
const int pin_BL = 10;
// LCD Buttons 
const int NONE = 0;
const int LEFT = 1;
const int RIGHT = 2;
const int UP = 3;
const int DOWN = 4;
const int SELECT = 5;
//Other pins
const int pin_buzzer = 3; // buzzer PIN
const int pin_WTS = 15; // Water temperature sensor PIN
//Room temperature 1 and 2 sensor PIN
const int pin_RT1 = 16;
const int pin_RT2 = 17;
//Relays pins
const int pin_WP = 18;  // Water pump pin relay
const int pin_H = 19;   // Heater pin relay

// Default values for the conrol functions
#ifdef SENSORDEBUG
const int DEFAULT_HONOFFTIME = 10000;   // test 10 sec. only
const int DEFAULT_WPONOFFTIME = 10000;  // test 10 sec. only
#else
const unsigned long DEFAULT_HONOFFTIME = 600000;    // Heater can't be turned on earlier than 10 mins after last turn off.
const unsigned long DEFAULT_WPONOFFTIME = 1200000;  // Once the pump is turned on it will run for at least 20 minutes. 
#endif

const int DEFAULT_WTPUMP = 4500;      // When water temp is higher than 45 degrees, the water pump turns on
const int BACKGLIGHTTIME = 20000;     // Turn off display backlight after 20s.
const unsigned long DATASENDPERIOD = 60000;      // The data will be send to serial port every 60 seconds. 

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(pin_WTS);
DallasTemperature sensors(&oneWire); // Pass our oneWire reference to Dallas Temperature.
// Initialize room temperature sensors
#define DHTTYPE DHT22        // DHT 22  (AM2302)
DHT dht1(pin_RT1, DHTTYPE);  // Initialize both DHT sensors
DHT dht2(pin_RT2, DHTTYPE);

// global variables 
LiquidCrystal lcd(pin_RS, pin_EN, pin_d4, pin_d5, pin_d6, pin_d7);
MyMenu main_menu(MAINMENUSIZE, MAINMENU, MMENTRIES_TABLE, MAINMENUSIZE);  // menu also stores measured values
MyMenu settings_menu(SETTINGSMENUSIZE, SETTINGSMENU, SMENTRIES_TABLE, SETTINGSMENUSIZE); // menu also stores configuration values 
MySerial serial_link(9600); // Initialize the serial class for sending and receiving the key value pairs

// Timer class is used to implement delays in some operations. 
class MyTimer {
  private:
    unsigned long start_time;
    unsigned long duration;
    bool running;

  public:
    MyTimer(unsigned long d, bool start = false) {
      duration = d;
      if (start) {
        start_timer();
      } else
        running = false;
    }

    bool expired() {
      if (millis() - start_time > duration) {
        running = false;
        return true;
      } else
        return !running;
    }
    void start_timer() {
      start_time = millis();
      running = true;
    }
};

// update the value of temp and humidity 
// converts the float value into integer (*100) and averages out too smoothen the values
// this is a helper function used in read_sensors() functions
void update_mainmenu_value (int item, float value) {
      if (value > SPECIALVALUE) {    //in case of error, do not change the value, keep the last one. 
        int d_value = (int)((4 * main_menu.value_get(item) + (int)(value * 100)) / 5); // approximately an average of last 5 values
        main_menu.value_set(item, d_value);
      }     
}

void read_sensors(bool immediate = false) {
 #ifdef SENSORDEBUG
  static bool firsttime = true;

  if (firsttime) {
    main_menu.value_set(ACTROOM1TEMP, DEFAULT_ROOM1_TEMP - 20);
    main_menu.value_set(ACTROOM2TEMP, DEFAULT_ROOM2_TEMP - 20);
    main_menu.value_set(ACTWATEROUTTEMP, DEFAULT_WTPUMP - 100);
    main_menu.value_set(ACTWATERINTEMP, DEFAULT_WTPUMP - 100);    
    firsttime = false;
  }
 #else
  // read water temperature and room temperature every 10 seconds
  static MyTimer T(5000);  // create a timer for 5 seconds and start it immediately
  
  if (T.expired()) {
    sensors.requestTemperatures();

    // Read water 1 temperature
    float Water1T = sensors.getTempCByIndex(1); // index to be tried and set when sensor changes
    if (Water1T != DEVICE_DISCONNECTED_C) 
      update_mainmenu_value(ACTWATEROUTTEMP, Water1T);
    else
      main_menu.value_set(ACTWATEROUTTEMP, ERROR);  // Error value

    // Read water 2 temperature
    float Water2T = sensors.getTempCByIndex(0); // index to be tried and set when sensor changes
    if (Water2T != DEVICE_DISCONNECTED_C) 
      update_mainmenu_value(ACTWATERINTEMP, Water2T);
    else
      main_menu.value_set(ACTWATERINTEMP,  ERROR);  // Error value

    // Read Rooms temperature and humidity
    float Room1H = dht1.readHumidity();
    float Room1T = dht1.readTemperature();
    if (isnan(Room1T)) 
      main_menu.value_set(ACTROOM1TEMP, ERROR);  // Error value 
    else 
      update_mainmenu_value(ACTROOM1TEMP, Room1T);

    if (isnan(Room1H))
      main_menu.value_set(ACTROOM1HUM, ERROR);  // Error value
    else 
      main_menu.value_set(ACTROOM1HUM, (int)(100 * Room1H));
    
    float Room2H = dht2.readHumidity();
    float Room2T = dht2.readTemperature();
    if (isnan(Room2T)) 
      main_menu.value_set(ACTROOM2TEMP, ERROR);  // Error value
    else 
      update_mainmenu_value(ACTROOM2TEMP, Room2T);
   
    if (isnan(Room2H)) 
      main_menu.value_set(ACTROOM2HUM, ERROR);  // Error value
    else
      main_menu.value_set(ACTROOM2HUM, (int)(100 * Room2H));
   
    T.start_timer();  // start the timer again
  }
 #endif
}

// Send data from sensors to Serial where WiFi module picks it up and sends to the Arduino cloud
// Sending it every DATASENDPERIOD seconds
void send_data(bool force = false) {
  static MyTimer T2(DATASENDPERIOD, true); // timer used to send data via serial every X seconds

  if (T2.expired() || force)
  {
    serial_link.SendKVPair({"T1", main_menu.value_get(ACTROOM1TEMP)});
    serial_link.SendKVPair({"T2", main_menu.value_get(ACTROOM2TEMP)});
    serial_link.SendKVPair({"H1", main_menu.value_get(ACTROOM1HUM)});
    serial_link.SendKVPair({"H2", main_menu.value_get(ACTROOM2HUM)});
    serial_link.SendKVPair({"WT1", main_menu.value_get(ACTWATEROUTTEMP)});
    serial_link.SendKVPair({"WT2", main_menu.value_get(ACTWATERINTEMP)});
    Serial.println("");
    if (force)
    {
      serial_link.SendKVPair({"TT1", settings_menu.value_get(SETROOM1TEMP)});
      serial_link.SendKVPair({"TT2", settings_menu.value_get(SETROOM2TEMP)});
      int afv = (settings_menu.value_get(SETANTIFREEZE) == OFF) ? 0 : 1;
      serial_link.SendKVPair({"AF", afv});
      Serial.println("");
    }
    T2.start_timer();
  }
}

/* program start */
void setup() {
  Serial.begin(9600);
  delay(1500);
  // initialize the LCD
  lcd.begin(16, 2);
  lcd.setCursor(0, 0);
  lcd.print("Inicializuji...");
  // setup the buzzer, pump and heater pins as output
  pinMode(pin_buzzer, OUTPUT);
  pinMode(pin_WP, OUTPUT);
  pinMode(pin_H, OUTPUT);
  // Turn off both relays and the buzzer
  digitalWrite(pin_buzzer, HIGH);
  digitalWrite(pin_WP, HIGH);
  digitalWrite(pin_H, HIGH);
  // Initiate the temperature libraries
  sensors.begin();
  sensors.setWaitForConversion(false);
  dht1.begin();  
  dht2.begin();  
  read_sensors();  // get the first reading before getting to the control loop to avoid turn on/off at the beginning. 
  // Read the settings from EEPROM
  ReadEEPROM();
  // send the settings to the cloud
  LOG(("Initial setting:"));
  send_data(true);
}

void backlight(int key) {
  static MyTimer T(BACKGLIGHTTIME, true); // timer to control display backlight. It turns off after BACKGLIGHTTIME seconds. 
  
  if (key != NONE) {
    pinMode(pin_BL, INPUT);
    T.start_timer();
  }
  else if (T.expired()) {
    pinMode(pin_BL, OUTPUT);
    digitalWrite(pin_BL, LOW);
  }
}

void receive_data() {
  keyvalue_pair_t kv;
  bool save2EEPROM = false;
  
  if (serial_link.ReceiveKVPair(kv)) {
    LOG("Data prijata z cloudu"); 
    
    if (strcmp(kv.key, "TT1") == 0) {
      settings_menu.value_set(SETROOM1TEMP, kv.value);
      save2EEPROM = true;
    }
    else if (strcmp(kv.key, "TT2") == 0) {
      settings_menu.value_set(SETROOM2TEMP, kv.value);
      save2EEPROM = true;
    }
    else if (strcmp(kv.key, "AF") == 0) {
      int afv = (kv.value == 0) ? OFF : ON;
      settings_menu.value_set(SETANTIFREEZE, afv);
      save2EEPROM = true;
    }
    
    if (save2EEPROM) {
        WriteEEPROM();
        LOG("Saving new settings from cloud to EEPROM"); 
    }   
  }
}

void loop() {
  // read temperature and humidity
  read_sensors();
  // handle key presses
  int key = read_key();
  // manage display backlight
  backlight(key);
  // manage rest 
  run_menu(key);
  // send measured data to cloud and read the settings from there
  send_data();
  receive_data();
  // Do the control logic
  control_system();
}

// Heater class is used to provide persistence that allows delays between switch on and off. 
class MyHeater {
  private:
    MyTimer T; // can't be turned on earlier than X mins after turn off.
    bool off;

  public:
    MyHeater() : T(DEFAULT_HONOFFTIME) {
      off = true;
    }

    void heater_on() {
      if (T.expired()) { 
        digitalWrite(pin_H, LOW);
        main_menu.value_set(ACTELHEATERON, ON);  
        off = false;      
      }
    }

    void heater_off() {
      if (!off) {
        digitalWrite(pin_H, HIGH);
        main_menu.value_set(ACTELHEATERON, OFF);
        T.start_timer();
        off = true;
       }
    }
};

// Pump class is used to provide persistence that allows delays between switch on and off. 
class MyPump {
  private:
    MyTimer T;  // Once the pump is turned on it will run for a preset number of minutes. 
  
  public:
    MyPump(unsigned long t = DEFAULT_WPONOFFTIME) : T(t) {
    }
    
    void pump_on() {
      digitalWrite(pin_WP, LOW);
      main_menu.value_set(ACTPUMPON, ON);
      T.start_timer();
    }

    void pump_off() {
      if (T.expired()) {
        digitalWrite(pin_WP, HIGH);
        main_menu.value_set(ACTPUMPON, OFF);
      }
    }
};

void start_alarm() {
  digitalWrite(pin_buzzer, LOW);
}

void stop_alarm() {
  digitalWrite(pin_buzzer, HIGH);
}

void ReadEEPROM() {
  int v;

  EEPROM.get(VPOSITION, v);
  if (v != VERSION) { // SW version change, force to apply defaults, not what is in the EEPROM. 
    EEPROM.put(VPOSITION, VERSION); // store new version number.
    WriteEEPROM(); // take defaults and write them all into EEPROM. This works only beacuse this function is called once, before anything else happens and defaults are in the settings. 
  }
  
  for (int i = 0; i < SETTINGSMENUSIZE - 1; i++) {    
    EEPROM.get(2 * i, v);
    if (v <= settings_menu.get_max(i) && v >= settings_menu.get_min(i))
      settings_menu.value_set(i, v);
  }
}

void WriteEEPROM() {
  for (int i = 0; i < SETTINGSMENUSIZE - 1; i++) 
    EEPROM.put(2 * i, settings_menu.value_get(i));
}

int read_key() {
  // returns key value only after it has been released
  static int lastkey_pressed = NONE;
  int r = NONE;
  unsigned long t;
  static MyTimer T(333);  // create time for 1/3 second

  int x = analogRead(0);

  if (x < 800 && lastkey_pressed == NONE)
    T.start_timer();

  if (x < 60)
    lastkey_pressed = RIGHT;
  else if (x < 200)
    lastkey_pressed = UP;
  else if (x < 400)
    lastkey_pressed = DOWN;
  else if (x < 600)
    lastkey_pressed = LEFT;
  else if (x < 800)
    lastkey_pressed = SELECT;
  else if (lastkey_pressed != NONE) {
    r = lastkey_pressed;
    lastkey_pressed = NONE;
  }
  if (T.expired()) {
    r = lastkey_pressed;
    lastkey_pressed = NONE;
  }

  return r;
}

void display_2lines(String Line1, String Line2) {
  // updates entire display with 2 lines on the input
  lcd.setCursor(0, 0);
  lcd.print(Line1);
  lcd.setCursor(0, 1);
  lcd.print(Line2);
}

void run_menu(int key) {
  static bool menu_type = MAIN_MENU;
  
  if (menu_type == MAIN_MENU) {
    display_2lines(main_menu.text_get(), main_menu.value_getastext());
  #ifdef SENSORDEBUG
  switch (key) {
    case DOWN:
      main_menu.next();
      break;
    case UP:
      main_menu.prev();
      break;
    case LEFT:
      if (!main_menu.last()) // selecting last menu item of configuration menu means going back to main menu
        main_menu.value_decrease(); 
      break;  
    case RIGHT:
      if (!main_menu.last()) 
        main_menu.value_increase();
      break;  
    case SELECT:
      if (main_menu.last()) {
        menu_type = SETTINGS_MENU;
        main_menu.beginning();
      }
    }
    #else
    if (key == DOWN || key == RIGHT)
      main_menu.next();
    else if (key == UP || key == LEFT)
      main_menu.prev();
    else if (key == SELECT && main_menu.last()) { // selecting last mainu menu item means moving to configuration menu
      menu_type = SETTINGS_MENU;
      main_menu.beginning();
    }
  #endif
  } else if (menu_type == SETTINGS_MENU) {
    display_2lines(settings_menu.text_get(), settings_menu.value_getastext());
    switch (key) {
      case DOWN:
        settings_menu.next();
        break;
      case UP:
        settings_menu.prev();
        break;
      case LEFT:
        if (!settings_menu.last()) // selecting last menu item of configuration menu means going back to main menu
          settings_menu.value_decrease(); 
        break;  
      case RIGHT:
        if (!settings_menu.last()) 
          settings_menu.value_increase();
        break;  
      case SELECT:
        if (settings_menu.last()) {
          menu_type = MAIN_MENU;
          settings_menu.beginning();
          // save into permanent storage
          WriteEEPROM();
          // save into the cloud
          LOG("Zmena nastaveni, posilam do Cloudu");
          serial_link.SendKVPair({"TT1", settings_menu.value_get(SETROOM1TEMP)});
          serial_link.SendKVPair({"TT2", settings_menu.value_get(SETROOM2TEMP)});
          int afv = (settings_menu.value_get(SETANTIFREEZE) == OFF) ? 0 : 1;
          serial_link.SendKVPair({"AF", afv});
          Serial.println("");
        }
    }
  }
}

void control_system() {
  static MyHeater HT;
  static MyPump P;
  int hystereze = settings_menu.value_get(SETHYSTER);  //  up and down difference. TBD Mozna by melo byt ve stupnich, nikoliv procentech.
  int t; 

  // Any water temp > Max water temp => run Alarm, else stop Alarm. This is to avoid boiling the water out of stove. 
  t = max(main_menu.value_get(ACTWATEROUTTEMP),main_menu.value_get(ACTWATERINTEMP));
  if (t != ERROR) {
    if (t >= settings_menu.value_get(SETMAXWATERTEMP))
      start_alarm();
    else if (t < settings_menu.value_get(SETMAXWATERTEMP))
      stop_alarm();
  }
  
  // Electric Heater ON, OFF, or AUTO. If Auto then thermostat is on, i.e. Room temp to match preset temp.
  // Master sensor is either ROOM #1 or ROOM #2 (with plumbing that must not freeze). Depending on master, either preset #1 or #2 is used as required temperature. 
  int master_sensor = ACTROOM1TEMP;
  int master_temp = SETROOM1TEMP;
  if (settings_menu.value_get(SETANTIFREEZE) == ON) {
    master_sensor = ACTROOM2TEMP;
    master_temp = SETROOM2TEMP;
  }
  switch (settings_menu.value_get(SETELHEATER)) {
    case ON:
      HT.heater_on();
      break;      
    case OFF:
      HT.heater_off();
      break;
    case AUTO:
      t = main_menu.value_get(master_sensor);
      if (t != ERROR) {
        if (t >= settings_menu.value_get(master_temp) + hystereze ||
            main_menu.value_get(ACTWATEROUTTEMP) >= settings_menu.value_get(SETMAXWATEROUTHEATER))
        HT.heater_off();
      else if (t < settings_menu.value_get(master_temp) - hystereze)
        HT.heater_on();          
      }     
      else
        HT.heater_off(); // if sensors do not work, fail back and turn off the electric heater
  }
  // Electric Pump ON, OFF, or AUTO. If Auto then pump is on if water temperature is higher then threshold. 
  switch (settings_menu.value_get(SETPUMP)) {
    case ON:
      P.pump_on();
      break;      
    case OFF:
      P.pump_off();
      break;
    case AUTO:
      // Turn on pump if water is over preset temp.
      t = main_menu.value_get(ACTWATEROUTTEMP);
      if (t != ERROR) {
        if (main_menu.value_get(ACTWATEROUTTEMP) > DEFAULT_WTPUMP)
          P.pump_on();
        else
          P.pump_off();
      }
      else
        P.pump_on();  // if sensors fail, fail back to turning on the pump (dont know what temperature the water has)
  }
}