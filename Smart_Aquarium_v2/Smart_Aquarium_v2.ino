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

//  Define relay pin out ------------------------------------------------------------------------------------------------------------------------------------------------------------------

byte Relay_1 = 7;         //Light relay
byte Relay_2 = 8;         //Co2 relay
byte Relay_3 = 9;         //AirPump relay
byte Relay_4 = 10;        //Filter relay
byte ACS712 = 14;         //ACS712 current sensor (30A)
byte Automode_LED = 2;    //The reason 
byte Manualmode_LED = 3;


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

//Some convenient flags
bool printflag1 = false;
bool printflag2 = false;
bool printflag3 = false;


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


//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
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
  mcs.addChannel(mcs_Current);
}

//Initialize MCS service and WiFi service -------------------------------------------------------------------------------------------------------------------------------------------------
void MCS_Init() {
  while (WL_CONNECTED != WiFi.status())         //Continuly conneting Wifi untill successful
    WiFi.begin(mySSID, myPSWD);                 //Connect WiFi
  Serial.println("WiFi connected!");
  MCS_addchannel();                             //add channel to MCS
  while (!mcs.connected())
    mcs.connect();                              //Connect to MCS
}

//Setup pin mode --------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Pin_Setup() {
  pinMode(Relay_1, OUTPUT);
  pinMode(Relay_2, OUTPUT);
  pinMode(Relay_3, OUTPUT);
  pinMode(Relay_4, OUTPUT);
  pinMode(ACS712, INPUT);
  pinMode(Automode_LED,OUTPUT);
  pinMode(Manualmode_LED,OUTPUT);
}

// Get relay state from MCS
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

// Check the device is online or not-----------------------------------------------------------------------------------------------------------------------------------------------------
void check_MCS_connection() {
  while (!mcs.connected())
    mcs.connect();
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
void Timer_Mode() {
  //Get now time from RTC
  LRTC.get();
  NowHour = LRTC.hour();
  NowMin = LRTC.minute();
  
  //Check current time is same as start time or not
  //if yes, then turn on the sepecific relay
  //if not, turn off the relay 
  if (NowHour == StartHour && NowMin == StartMin){
    digitalWrite(Relay_1,HIGH);   //turn on light relay
    digitalWrite(Relay_2,HIGH);   //turn on Co2 relay
  }
  if (NowHour == EndHour && NowMin == EndMin){
    digitalWrite(Relay_1,LOW);    //turn off light relay
    digitalWrite(Relay_2,LOW);    //turn off Co2 relay
  }
}

// Initialize the RTC and set to real time
void RTC_Init() {
  LRTC.begin();
  LRTC.set(2020, 5, 5, NowHour, NowMin, NowSec);  //Set the RTC time to 2020/05/05/ HH:MM:SS (NTP time) the date of RTC is not importand, so I ignore the process to update real date.
  LRTC.get();
  //Serial.print("Nowtime ");
  //Serial.print(LRTC.hour());
  //Serial.print(LRTC.minute());
  //Serial.println(LRTC.second());
}

// Initialize NTP server to get correct time from internet
void NTP_Init() {
  Udp.begin(localPort);
  // First,send an NTP packet to a time server,
  // and wait to see if a reply is available.
  sendNTPpacket(NTP_server);

  delay(5000);
  if (Udp.parsePacket()) {
    // We've received a packet, read the data into the buffer
    Udp.read(packetBuffer, NTP_PACKET_SIZE);

    //Second,the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. we need to esxtract the two words:
    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);

    // And combine the four bytes (two words) into a long integer
    unsigned long secsSince1900 = highWord << 16 | lowWord;

    // this is NTP time (seconds since Jan 1 1900):
    // But we need to convert NTP time into everyday time:
    const unsigned long seventyYears = 2208988800UL;              //Unix time starts on Jan 1 1970. In seconds, that's 2208988800 second

    unsigned long NowTime = secsSince1900 - seventyYears;         //get the current time from 1970/01/01 (second)

    //Updete time parameter into NTP time
    NowHour = (NowTime  % 86400L) / 3600 + TimeZone;              // Convert into hour (86400 equals secs per day)
    NowMin = (NowTime  % 3600) / 60;
    NowSec = (NowTime % 60);
  }
}

// send an NTP request to the time server at the given address----------------------------------------------------------------------------------------------------------------------------
unsigned long sendNTPpacket(const char* host) {
  
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  
  // Initialize values needed to form NTP request
  // (see NTP Wiki above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;            // Stratum, or type of clock
  packetBuffer[2] = 6;            // Polling Interval
  packetBuffer[3] = 0xEC;         // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(host, 123); //NTP requests are to port 123

  Udp.write(packetBuffer, NTP_PACKET_SIZE);

  Udp.endPacket();

  return 0;
}

//Measure the current and send to MCS ---------------------------------------------------------------------------------------------------------------------------------------------------------
void get_AC_Current(){
  int analogVal = 0;              //Analog value read from the sensor pin
  int maxValue = 0;               //store max value, choose 0 to be the initial value
  int minValue = 4096;            //store min value, choose your microcontroller's "analog read max value" to be initial value (Arduino -> 1024)
  unsigned long CurrentTimer = millis();
  while((millis() - CurrentTimer) < 200){             //Sampling sensor value in 200mS period
      analogVal = analogRead(ACS712);
    if (analogVal > maxValue)                         //update max value
      maxValue = analogVal;
    if (analogVal < minValue)                         //update min value
      minValue = analogVal;
  }
  //Serial.print("Max val =");
  //Serial.print(maxValue);
  //Serial.print("/t");
  //Serial.print("Min val =");
  //Serial.println(minValue);
  float Vpp = (maxValue - minValue) * 5000 / 4096;    //Means peak-to-peak voltage is the max value - min value, and convert to the real voltage of ACS712 module, not ADC number.
                                                      //So the Vcc should be 5000mV instead of 2500mV.
  
  /* Limitations of Linkit7697 https://docs.labs.mediatek.com/resource/linkit7697-arduino/en/developer-guide/limitations-of-linkit-7697
   * Linkit 7697's ADC resolution is 4096 steps, and the voltage range is from 0 ~ 2.5V, here comes a problem:
   * Is the "Vpp" value calculate by the formula above correct or not?
   * The answer is NOT correct.
   * Because the neural point of module is 2.5V, is the maximum value of ADC input.
   * To completely fix this problem, you need to use two resisters (about 1k ohm) to make a voltage diviter, convert 5V to 2.5V proportionally, makes nural point to be 1.25V. 
   */
   
  Current = (Vpp / 2.0 * 0.707 / sensitivity) - Current_offset;           //Convert Vpp into Vrms, this value is the sensor's Vrms, and also represent current
  if(Current < 0)                                                         //Current is positive
    Current = 0;
  //Serial.print("Current =");
  //Serial.println(Current);
  mcs_Current.set(Current);                                               //send current to MCS
  Energe += (Volt * Current / 3600) / 1000;                               //Calculate power consumption and accumulate the energe. 
  Power.set(Energe);                                                      //Send power consumption to MCS -> P = I*V (W), W = P * T (kW/H)
}



//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------


void setup() {
  Serial.begin(115200);
  Pin_Setup();
  MCS_Init();   //MCS_Init() include the wifi initial process, be the first process in setup
  NTP_Init();   //NTP_Init() update current time into RTC module, need to be 
  RTC_Init();
  
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
  mcs.process(500);
  if (AutoMode.value()) {
    if(!printflag1){
      Serial.println("Auto Mode");
      digitalWrite(Automode_LED,HIGH);
      digitalWrite(Manualmode_LED,LOW);
    }
    printflag1 = true;
    printflag2 = false;
    
    get_SetTime();
    Timer_Mode();
  }
  else {
    if(!printflag2){
     Serial.println("Manual Mode");
     digitalWrite(Automode_LED,LOW);
     digitalWrite(Manualmode_LED,HIGH);
    }
    printflag2 = true;
    printflag1 = false;
   
    getStateFromMCS();
  }
  get_AC_Current();
  check_MCS_connection();
}
