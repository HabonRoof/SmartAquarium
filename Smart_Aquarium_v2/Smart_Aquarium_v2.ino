/*
   Smart Aquarium V2.0
   2020/05/03
   https://www.instagram.com/habonroof/
   for more infomation, please visit my personal website:https://sites.google.com/ntut.org.tw/habonroofplayground-en/home
*/

// Include necessary library for MCS service
#include <LWiFi.h>
#include <WiFiClient.h>
#include "MCS.h"

// Include RTC library
#include <LRTC.h>

//Include WiFi UDP library to use UDP service to get NTP time from internet
#include <WiFiUDP.h>

//Include Dallas DS18B20 library
#include <OneWire.h>
#include <DallasTemperature.h>

//Include I2C LCD library
#include <LiquidCrystal_I2C.h>

//Assign temperature sensor data wire pin
#define one_wire_bus 4

//Assign AP ssid and password
#define mySSID "johnson"
#define myPSWD "j8701292929"

//Assign your test device id / key
//You can find it on your test device website
MCSDevice mcs ("DbSzg0JB", "B9XagSJyEAhjBQrk");


//  MCS Channel Zone-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
/*
  Assign your customize channel id
  How many channels your test device has,
  here should have the same amount channel id to transmit the data to MCS
  Be aware to the channel id type, may be Controller or Display
  And the name of channel id is use for downer code, not for MCS service website.
  For example:
  MCSControllerOnOff  Name_of_channel_id ("real_channel_id");
*/
MCSControllerOnOff  Light("Relay_1");
MCSControllerOnOff  Co2("Relay_2");
MCSControllerOnOff  AirPump("Relay_3");
MCSControllerOnOff  Filter("Relay_4");
MCSControllerOnOff  AutoMode("Auto_Mode");
MCSControllerInteger setHour("StartTime_hour");
MCSControllerInteger setMin("StartTime_min");
MCSControllerInteger Duration("Duration");
MCSDisplayOnOff     Light_state("RelayState_1");
MCSDisplayOnOff     Co2_state("RelayState_2");
MCSDisplayOnOff     AirPump_state("RelayState_3");
MCSDisplayOnOff     Filter_state("RelayState_4");
MCSDisplayFloat     Power("Eng");
MCSDisplayFloat     Temp("Temp");
MCSDisplayFloat     mcs_Current("Current");

//  NTP setup zone-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
/*
  For more information about NTP, please vsit:https://en.wikipedia.org/wiki/Network_Time_Protocol
  This part of code is refer by Mediatek official example,
  You can find it at Arduino->File->Examples->Examples for Linkit7697->.LWiFi->WiFiUdpNtpClient
*/

unsigned int localPort = 2390;    //local port to listen for UDP packets, this port is located on Linkit7697

//Change this address depend on your location to get the more accurate time from NTP server
IPAddress timeServer(118, 163, 81, 61);         //Taiwan standard time NTP server IP address
const char *NTP_server = "time.stdtime.gov.tw"; //Use

const int TimeZone = 8;

const int NTP_PACKET_SIZE = 48;   // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets

// Create a UDP object, a UDP instance to let us send and receive packets over UDP
WiFiUDP Udp;

// Setup onewire bus for DS18B20 ----------------------------------------------------------------------------------------------------------------------------------------------------------
OneWire onewire(one_wire_bus);        //create onewire bus object named onewire
DallasTemperature sensors(&onewire);  //pass onewire object into DallasTemperature object named sensors
float Temperature = 0;

// Setup I2C LCD address
LiquidCrystal_I2C lcd(0x27);          //set LiquidCrystal_I2C object named lcd, address is 0x27
// I2C of Linkit 7697: SDA -> P9  SCL -> P8
//  Define relay pin out ------------------------------------------------------------------------------------------------------------------------------------------------------------------
//Pin 8 is I2C SCL, Pin9 is I2C SDA, connect to I2C perpherial (LCD)
const byte Relay_1 = 10;          //Light relay
const byte Relay_2 = 11;          //Co2 relay
const byte Relay_3 = 12;          //AirPump relay
const byte Relay_4 = 13;          //Filter relay
const byte ACS712 = 14;           //ACS712 current sensor (30A)
const byte Automode_LED = 2;      //The reason not use pin 0 and 1 is because they are serial bus TX and RX
const byte Manualmode_LED = 3;


//  Define time parameter -----------------------------------------------------------------------------------------------------------------------------------------------------------------
//the light will be open if Nowtime in the range of duration
//For example:
//Start |Hour:Minute| -> |Duration(minute)| -> End |Hour:Minute|
//          OFF                 ON                      OFF
//Start |  08:00    | -> |    480min      | -> End |   16:00   |

byte StartHour = 0;     byte StartMin = 0;
byte EndHour = 0;       byte EndMin = 0;
byte NowHour = 0;       byte NowMin = 0;
byte NowSec = 0;        int OpeningTime = 0;
byte last_update_minute = 0;

// Define current sensor parameter --------------------------------------------------------------------------------------------------------------------------------------------------------
//Sensitivity means how much mV when 1A current flow through the sensor
//Reference to the official datasheet:https://www.sparkfun.com/datasheets/BreakoutBoards/0712.pdf
//5A measurement -> 185mV/A | 20A measurement -> 100mV/A | 30A measurement -> 66mV/A

const int sensitivity = 66;
float Current = 0;            //Current value
float Current_offset = 0.21;  //Constant error of current sensor, about 0.22A

//parameter to calculate power consumption-------------------------------------------------------------------------------------------------------------------------------------------------
const int Volt = 110;           //Voltage of your home's electronic power
float Energe = 0;               //Energe consumption

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

//Some convenient flags for serial print or LCD
bool LEDflag1 = false;
bool LEDflag2 = false;
bool LEDflag3 = false;

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void MCS_Init();
void RTC_Init();
void NTP_Init();
void LCD_Init();
void mcs_Addchannel();
void pin_setup();
void get_status_from_MCS();
void get_SetTime();
void timer_Mode();
void sendNTPpacket();
void check_MCS_connection();
void lcd_print_Automode_info();
void lcd_print_Manulemode_info();
void mcs_update_dataPoint();


//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//Add channels to MCS service, the name of channel id is above the code, not on the MCS website.
void mcs_Addchannel() {
  mcs.addChannel(Light);
  mcs.addChannel(Light_state);
  mcs.addChannel(Co2);
  mcs.addChannel(Co2_state);
  mcs.addChannel(AirPump);
  mcs.addChannel(AirPump_state);
  mcs.addChannel(Filter);
  mcs.addChannel(Filter_state);
  mcs.addChannel(AutoMode);
  mcs.addChannel(setHour);
  mcs.addChannel(setMin);
  mcs.addChannel(Duration);
  mcs.addChannel(Power);
  mcs.addChannel(Temp);
  mcs.addChannel(mcs_Current);
}

//Setup pin mode --------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void pin_setup() {
  pinMode(Relay_1, OUTPUT);
  pinMode(Relay_2, OUTPUT);
  pinMode(Relay_3, OUTPUT);
  pinMode(Relay_4, OUTPUT);
  pinMode(ACS712, INPUT);
  pinMode(Automode_LED, OUTPUT);
  pinMode(Manualmode_LED, OUTPUT);
}

// Get relay state from MCS ---------------------------------------------------------------------------------------------------------------------------------------------------------------
void get_status_from_MCS() {
  //Get the switch state from MCS
  //and update the state to MCS display
  if (Light.updated()) {
    digitalWrite(Relay_1, Light.value() ? HIGH : LOW);
    Light_state.set(Light.value());
  }
  if (Co2.updated()) {
    digitalWrite(Relay_2, Co2.value() ? HIGH : LOW);
    Co2_state.set(Co2.value());
  }
  if (AirPump.updated()) {
    digitalWrite(Relay_3, AirPump.value() ? HIGH : LOW);
    AirPump_state.set(AirPump.value());
  }
  if (Filter.updated()) {
    digitalWrite(Relay_4, Filter.value() ? HIGH : LOW);
    Filter_state.set(Filter.value());
  }
}



// Get setting time from MCS ------------------------------------------------------------------------------------------------------------------------------------------------------------
void get_SetTime() {
  //Get set time from MCS, include the start time(hour), start time(minute), and opening time
  StartHour = setHour.value();
  StartMin = setMin.value();
  OpeningTime = Duration.value();
  //Calculate the end time
  EndHour = StartHour + (OpeningTime / 60);
  EndMin = StartMin + (OpeningTime % 60);
  //If end time(hour) is greater than 24, that means the end time is at next day, so we need reset the hour to prevent error
  if (EndHour >= 24)
    EndHour = EndHour - 24;
  //Same, if the end time(minute) is more than 60, means the end time need a carry to next hour.
  if (EndMin >= 60) {
    EndMin = EndMin - 60;
    EndHour += EndHour;
  }
}

// When Auto mode is on, goin to timer relay --------------------------------------------------------------------------------------------------------------------------------------------
void timer_Mode() {
  //Get now time from RTC
  LRTC.get();
  NowHour = LRTC.hour();
  NowMin = LRTC.minute();

  //Check current time is same as start time or not
  //if yes, then turn on the sepecific relay
  //if not, turn off the relay
  if (NowHour == StartHour && NowMin == StartMin) {           //Small bug : If now time is 12:00, but start time is 8:00 end time is 16:00 the relay should not be open
    digitalWrite(Relay_1, HIGH);  //turn on light relay
    digitalWrite(Relay_2, HIGH);  //turn on Co2 relay
  }
  if (NowHour == EndHour && NowMin == EndMin) {
    digitalWrite(Relay_1, LOW);   //turn off light relay
    digitalWrite(Relay_2, LOW);   //turn off Co2 relay
  }
}

// Get aquarium temperature via DS18B20 --------------------------------------------------------------------------------------------------------------------------------------------------
void get_Temperature() {
  sensors.requestTemperatures();                //Request all temperature sensor's value
  if (sensors.getTempCByIndex(0) < 0)           //Avoid error data (-127) caused by poor contact, use one if conditional to check the data
    Temperature = Temperature;
  else
    Temperature = sensors.getTempCByIndex(0);   //get the temperature in Celsius at sensor index "0"
  // Temp.set(Temperature);                     //Update temperature to MCS (move to mcs_update_datapoint function)
  Serial.print("Temperature:");
  Serial.println(Temperature);
}

// Update data to MCS server per minutes to reduce usage of datapoint
void mcs_update_dataPoint() {
  LRTC.get();
  NowMin = LRTC.minute();
  if (NowMin - last_update_minute == 1) {             //To reduce MCS datapoint usage, update datapoint per minute
    Serial.println("Update to MCS per minute");
    mcs_Current.set(Current);
    Power.set(Energe);
    Temp.set(Temperature);
    
  }
  LRTC.get();
  last_update_minute = LRTC.minute();
}
// Check the device is online or not-----------------------------------------------------------------------------------------------------------------------------------------------------
void check_MCS_connection() {
  while (!mcs.connected())
    mcs.connect();
}
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------


void setup() {
  Serial.begin(115200);
  LCD_Init();
  pin_setup();
  MCS_Init();   //MCS_Init() include the wifi initial process, be the first process in setup
  NTP_Init();   //NTP_Init() update current time into RTC module, need to be execute before RTC_Init()
  RTC_Init();   //Set RTC time to reality real time

  //If the data is invalid, reget data from MCS
  while (!Light.valid() && !Co2.valid() && !AirPump.valid() && !Filter.valid() && !AutoMode.valid()) {
    Light.value();
    Co2.value();
    AirPump.value();
    Filter.value();
    AutoMode.value();
  }
  sensors.begin();        //Start DallasTemperature sensors
}


void loop() {
  //call process() to allow background processing, add timeout to avoid high cpu usage
  mcs.process(500);
  if (AutoMode.value()) {
    if (!LEDflag1) {
      Serial.println("Auto Mode");
      digitalWrite(Automode_LED, HIGH);
      digitalWrite(Manualmode_LED, LOW);
    }
    LEDflag1 = true;
    LEDflag2 = false;

    get_SetTime();              //Get the setting time from MCS via input box
    timer_Mode();               //Start timer mode, check now need to open or close the relay(s)
    lcd_print_Automode_info();
  }
  else {
    if (!LEDflag2) {
      Serial.println("Manual Mode");
      digitalWrite(Automode_LED, LOW);
      digitalWrite(Manualmode_LED, HIGH);
    }
    LEDflag2 = true;
    LEDflag1 = false;

    lcd_print_Manulemode_info();
    get_status_from_MCS();          //Control relays through MCS pannel
  }
  get_AC_Current();             //Refresh curremt meter value in main loop
  get_Temperature();            //Refresh temperature value in main loop
  mcs_update_dataPoint();       //Update data per minute
  check_MCS_connection();       //Check MCS connection in each loop
}

void lcd_print_Automode_info() {
  lcd.clear();
  lcd.print("Tmp:");
  lcd.setCursor(4, 0);
  lcd.print(Temperature);
  lcd.setCursor(8, 0);
  lcd.print("Cur:");
  lcd.setCursor(12, 0);
  lcd.print(Current);
  if (NowHour < 10) {
    lcd.setCursor(0, 1);
    lcd.print("0");
    lcd.setCursor(1, 1);
    lcd.print(NowHour);
  }
  else {
    lcd.setCursor(0, 1);
    lcd.print(NowHour);
  }
  lcd.setCursor(2, 1);
  lcd.print(":");
  if (NowMin < 10) {
    lcd.setCursor(3, 1);
    lcd.print("0");
    lcd.setCursor(4, 1);
    lcd.print(NowMin);
  }
  else {
    lcd.setCursor(3, 1);
    lcd.print(NowMin);
  }

  if (StartHour < 10) {
    lcd.setCursor(5, 1);
    lcd.print("0");
    lcd.setCursor(6, 1);
    lcd.print(StartHour);
  }
  else {
    lcd.setCursor(5, 1);
    lcd.print(StartHour);
  }
  lcd.setCursor(7, 1);
  lcd.print(":");
  if (StartMin < 10) {
    lcd.setCursor(8, 1);
    lcd.print("0");
    lcd.setCursor(9, 1);
    lcd.print(StartMin);
  }
  else {
    lcd.setCursor(8, 1);
    lcd.print(StartMin);
  }
  lcd.setCursor(10, 1);
  lcd.print(">");
  if (EndHour < 10) {
    lcd.setCursor(11, 1);
    lcd.print("0");
    lcd.setCursor(12, 1);
    lcd.print(EndHour);
  }
  else {
    lcd.setCursor(11, 1);
    lcd.print(EndHour);
  }
  lcd.setCursor(13, 1);
  lcd.print(":");
  if (EndMin < 10) {
    lcd.setCursor(14, 1);
    lcd.print("0");
    lcd.setCursor(15, 1);
    lcd.print(EndMin);
  }
  else {
    lcd.setCursor(14, 1);
    lcd.print(EndMin);
  }
}

void lcd_print_Manulemode_info() {
  lcd.clear();
  lcd.print("Tmp:");
  lcd.setCursor(4, 0);
  lcd.print(Temperature);
  lcd.setCursor(8, 0);
  lcd.print("Cur:");
  lcd.setCursor(12, 0);
  lcd.print(Current);
  if (NowHour < 10) {
    lcd.setCursor(0, 1);
    lcd.print("0");
    lcd.setCursor(1, 1);
    lcd.print(NowHour);
  }
  else {
    lcd.setCursor(0, 1);
    lcd.print(NowHour);
  }
  lcd.setCursor(2, 1);
  lcd.print(":");
  if (NowMin < 10) {
    lcd.setCursor(3, 1);
    lcd.print("0");
    lcd.setCursor(4, 1);
    lcd.print(NowMin);
  }
  else {
    lcd.setCursor(3, 1);
    lcd.print(NowMin);
  }
}
