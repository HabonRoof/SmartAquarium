
//Initialize MCS service and WiFi service -------------------------------------------------------------------------------------------------------------------------------------------------
void MCS_Init() {
  while (WL_CONNECTED != WiFi.status())         //Continuly conneting Wifi untill successful
    WiFi.begin(mySSID, myPSWD);                 //Connect WiFi
  Serial.println("WiFi connected!");
  mcs_Addchannel();                             //add channel to MCS
  while (!mcs.connected())
    mcs.connect();                              //Connect to MCS
}

// Initialize the RTC and set to real time ----------------------------------------------------------------------------------------------------------------------------------------------
void RTC_Init() {
  LRTC.begin();
  LRTC.set(2020, 5, 10, NowHour, NowMin, NowSec);  //Set the RTC time to 2020/05/05/ HH:MM:SS (NTP time) the date of RTC is not importand, so I ignore the process to update real date.
  LRTC.get();
  Serial.print("Nowtime ");
  Serial.print(LRTC.hour());
  Serial.print(":");
  Serial.print(LRTC.minute());
  Serial.print(":");
  Serial.println(LRTC.second());
}

// Initialize NTP server to get correct time from internet ------------------------------------------------------------------------------------------------------------------------------
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

// Initialize the LCD screen -------------------------------------------------------------------------------------------------------------------------------------------------------------
void LCD_Init(){
  lcd.begin(16,2);
  lcd.setCursor(7,0);
  lcd.print("IOT");
  lcd.setCursor(4,1);
  lcd.print("AQUARIUM");
  delay(1000);
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
  //mcs_Current.set(Current);                                               //send current to MCS (move to mcs_update_datapoint function)
  Energe += (Volt * Current / 3600) / 1000;                               //Calculate power consumption and accumulate the energe. 
  //Power.set(Energe);                                                      //Send power consumption to MCS -> P = I*V (W), W = P * T (kW/H) (move to mcs_update_datapoint function)
}
