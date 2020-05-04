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

MCSDevice mcs("DbSzg0JB", "B9XagSJyEAhjBQrk");

//MCS Channel Zone-------------------------------------------------------------------------------------------------------------------------------------------------------------------------

//Assign your customize channel id
//How many channels your test device has,
//here should have the same amount channel id to transmit the data to MCS
//Be aware to the channel id type, may be Controller or Display
//And the name of channel id is use for downer code, not for MCS service website.
//For example 
//MCSControllerOnOff  Name_of_channel_id ("real_channel_id");

MCSControllerOnOff  Light("Relay_1");
MCSControllerOnOff  Co2("Relay_2");
MCSControllerOnOff  AirPump("Relay_3");
MCSControllerOnOff  Filter("Relay_4");
MCSControllerOnOff  AutoMode("Auto_Mode");
MCSControllerInteger setHour("StartTime_hour");
MCSControllerInteger setMin("StartTime_min");
MCSControllerInteger duration("Duration");

MCSDisplayOnOff     Light_state("RelayState_1");
MCSDisplayOnOff     Co2_state("RelayState_2");
MCSDisplayOnOff     AirPump_state("RelayState_3");
MCSDisplayOnOff     Filter_state("RelayState_4");
MCSDisplayFloat     Power("Eng");
MCSDisplayFloat     Temp("Temp");

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

//define pin out
byte Relay_1 = 0;
byte Relay_2 = 0;
byte Relay_3 = 0;
byte Relay_4 = 0;

void MCS_addchannel();
void MCS_init();


//Add channels to MCS service, the name of channel id is above the code, not on the MCS website.

void MCS_addchannel(){          
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

void MCS_init() {
  while (WL_CONNECTED != WiFi.status()) {       //Continuly conneting Wifi untill successful
    Serial.print("WiFi.begin(");
    Serial.print(_SSID);
    Serial.print(",");
    Serial.print(_KEY);
    Serial.println(")...");
    WiFi.begin(_SSID, _KEY);
  }
  Serial.println("WiFi connected!");
  MCS_addchannel();
  while(!mcs.connected()){
    Serial.println("MCS connecting...");
    mcs.connect();
  }

}


void setup() {
  MCS_init();

}

void loop() {


}
