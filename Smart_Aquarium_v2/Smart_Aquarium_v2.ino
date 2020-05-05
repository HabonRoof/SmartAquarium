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

//Assign AP ssid and password
#define mySSID "johnson"
#define myPSWD "j8701292929"

//Assign your test device id / key
//You can find it on your test device website
MCSDevice mcs ("DbSzg0JB", "B9XagSJyEAhjBQrk");


//MCS Channel Zone-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
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

//NTP setup zone--------------------------------------------------------------------------------------------------------------------------------------------------------------------------
/*
  For more information about NTP, please vsit:https://en.wikipedia.org/wiki/Network_Time_Protocol
  This part of code is refer by Mediatek official example,
  You can find it at Arduino->File->Examples->Examples for Linkit7697->.LWiFi->WiFiUdpNtpClient
*/

unsigned int localPort = 2390;    //local port to listen for UDP packets, this port is located on Linkit7697

//Change this address depend on your location to get the more accurate time from NTP server
IPAddress timeServer(118, 163, 81, 61);       //Taiwan standard time NTP server IP address
const char *NTP_server = "time.stdtime.gov.tw";

const int TimeZone = 8;

const int NTP_PACKET_SIZE = 48;   // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets

// Create a UDP object, a UDP instance to let us send and receive packets over UDP
WiFiUDP Udp;

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------


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
void RTC_Init();
void NTP_Init();
void sendNTPpacket();

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

void get_SetTime() {
  //Get set time from MCS
  StartHour = setHour.value();
  StartMin = setMin.value();
  OpeningTime = Duration.value();
  EndHour = StartHour + (OpeningTime / 60);
  EndMin = StartMin + (OpeningTime % 60);
  //If tend time(hour) is greater than 24, that means the end time is at next day, so we need reset the hour to prevent error
  if (EndHour >= 24)
    EndHour = EndHour - 24;
  //Same, if the end time(minute) is more than 60, means the end time need to carry to next hour.
  if (EndMin >= 60) {
    EndMin = EndMin - 60;
    EndHour += EndHour;
  }
}

void Timer_Mode() {

}

void RTC_Init() {
  LRTC.begin();
  LRTC.get();
  //Put NTP time here
  //
  //
  LRTC.hour();
  LRTC.minute();
  LRTC.second();
}



void NTP_Init(){
  Udp.begin(localPort);
  sendNTPpacket(NTP_server);// send an NTP packet to a time server
  // wait to see if a reply is available
  delay(1000);
  if (Udp.parsePacket()) {
    Serial.println("packet received");
    // We've received a packet, read the data from it
    Udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

    //the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, esxtract the two words:

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    Serial.print("Seconds since Jan 1 1900 = ");
    Serial.println(secsSince1900);

    // now convert NTP time into everyday time:
    Serial.print("Unix time = ");
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;
    // subtract seventy years:
    unsigned long epoch = secsSince1900 - seventyYears;
    // print Unix time:
    Serial.println(epoch);


    // print the hour, minute and second:
    Serial.print("The UTC time is ");       // UTC is the time at Greenwich Meridian (GMT)
    Serial.print((epoch  % 86400L) / 3600 + gxz  TimeZone); // print the hour (86400 equals secs per day)
    Serial.print(':');
    if (((epoch % 3600) / 60) < 10) {
      // In the first 10 minutes of each hour, we'll want a leading '0'
      Serial.print('0');
    }
    Serial.print((epoch  % 3600) / 60); // print the minute (3600 equals secs per minute)
    Serial.print(':');
    if ((epoch % 60) < 10) {
      // In the first 10 seconds of each minute, we'll want a leading '0'
      Serial.print('0');
    }
    Serial.println(epoch % 60); // print the second
  }
}

// send an NTP request to the time server at the given address----------------------------------------------------------------------------------------------------------------------------
unsigned long sendNTPpacket(const char* host) {
  //Serial.println("1");
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  //Serial.println("2");
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  //Serial.println("3");

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(host, 123); //NTP requests are to port 123
  //Serial.println("4");
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  //Serial.println("5");
  Udp.endPacket();
  //Serial.println("6");
  return 0;
}
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------


void setup() {
  Serial.begin(9600);
  Pin_Setup();
  RTC_Init();
  MCS_Init();
  NTP_Init();
  //If the data is invalid, reget data from MCS
  while (!Light.valid() && !Co2.valid() && !AirPump.valid() && !Filter.valid() && !AutoMode.valid()) {
    Light.value();
    Co2.value();
    AirPump.value();
    Filter.value();
    AutoMode.value();
  }
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
