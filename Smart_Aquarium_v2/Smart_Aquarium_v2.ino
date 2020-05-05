/*
   Smart Aquarium V2.0
   2020/05/03
   https://www.instagram.com/habonroof/

   for more infomation, please visit my personal website:
*/

// Include necessary library for MCS service
#include <LWiFi.h>
#include <WiFiClient.h>
#include "MCS.h"

//Assign AP ssid and password
#define mySSID "johnson"
#define myPSWD "j8701292929"

//Assign your test device id / key
//You can find it on your test device website

MCSDevice mcs ("DbSzg0JB", "B9XagSJyEAhjBQrk");

//MCS Channel Zone-------------------------------------------------------------------------------------------------------------------------------------------------------------------------

//Assign your customize channel id
//How many channels your test device has,
//here should have the same amount channel id to transmit the data to MCS
//Be aware to the channel id type, may be Controller or Display
//And the name of channel id is use for downer code, not for MCS service website.
//For example:
//MCSControllerOnOff  Name_of_channel_id ("real_channel_id");
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

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------


//Define relay pin out
byte Relay_1 = 7;
byte Relay_2 = 8;
byte Relay_3 = 9;
byte Relay_4 = 10;

//Define time parameter-------------------------------------------------------------------------------------------------------------------------------------------------------------------
//the light will be open if Nowtime in the range of duration
//For example:
//Start |Hour:Minute| -> |Duration(minute)| -> End |Hour:Minute|
//          OFF                 ON                      OFF
//Start |  08:00    | -> |    480min      | -> End |   16:00   |

byte StartHour = 0;     byte StartMin = 0;
byte EndHour = 0;       byte EndMin = 0;
byte NowHour = 0;       byte NowMin = 0;
int OpeningTime = 0;

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void MCS_addchannel();
void MCS_Init();
void Pin_Setup();
void getStateFromMCS();
void check_MCS_connection();
void get_SetTime();
void Timer_Mode();

//Add channels to MCS service, the name of channel id is above the code, not on the MCS website.
void MCS_addchannel() {
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
}


//Initialize MCS service and WiFi service
void MCS_Init() {
  while (WL_CONNECTED != WiFi.status()) {       //Continuly conneting Wifi untill successful
    Serial.print("WiFi.begin(");
    WiFi.begin(mySSID, myPSWD);                 //connect wifi
  }
  Serial.println("WiFi connected!");
  MCS_addchannel();                             //add channel to MCS
  while (!mcs.connected()) {
    Serial.println("MCS connecting...");
    mcs.connect();                              //Connect to MCS
  }
  Serial.println("MCS connected");
}

void Pin_Setup() {
  pinMode(Relay_1, OUTPUT);
  pinMode(Relay_2, OUTPUT);
  pinMode(Relay_3, OUTPUT);
  pinMode(Relay_4, OUTPUT);
}

void getStateFromMCS() {
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

void check_MCS_connection() {
  while (!mcs.connected())
  {
    Serial.println("re-connect to MCS...");
    mcs.connect();
    if (mcs.connected())
      Serial.println("MCS connected !!");
  }
}

void get_SetTime(){
  //Get set time from MCS
  StartHour = setHour.value();
  StartMin = setMin.value();
  OpeningTime = Duration.value();
  EndHour = StartHour + (OpeningTime / 60);
  EndMin = StartMin + (OpeningTime % 60);
  //If tend time(hour) is greater than 24, that means the end time is at next day, so we need reset the hour to prevent error
  if(EndHour >= 24)
    EndHour = EndHour - 24;
  //Same, if the end time(minute) is more than 60, means the end time need to carry to next hour.
  if(EndMin >= 60){
    EndMin = EndMin -60;
    EndHour += EndHour;
  }
}

void Timer_Mode(){
  
}


void setup() {
  Pin_Setup();
  MCS_Init();
  //If the data is invalid, reget data from MCS-------------------------------------------------------------------------------------------------------------------------------------------
  while (!Light.valid() && !Co2.valid() && !AirPump.valid() && !Filter.valid() && !AutoMode.valid()) {
    Light.value();
    Co2.value();
    AirPump.value();
    Filter.value();
    AutoMode.value();
  }
  //--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
  getStateFromMCS();
}

void loop() {
  //call process() to allow background processing, add timeout to avoid high cpu usage
  mcs.process(100);
  if (AutoMode.value()) {
    get_SetTime();
    Timer_Mode();
  }
  else {
    getStateFromMCS();
  }
  check_MCS_connection();
}
