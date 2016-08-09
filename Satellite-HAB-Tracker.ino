#include <IridiumSBD.h> //import required libraries into sketch
#include <Adafruit_GPS.h>
#include <SoftwareSerial.h>

SoftwareSerial nss(11, 12); //set up serial pins
IridiumSBD isbd(nss, 10); //attach object to serial pins
static const int ledPin = 13;
SoftwareSerial mySerial(8, 9);
Adafruit_GPS GPS(&mySerial);

#define GPSECHO  true //this will send received GPS strings straight to the serial monitor

void setup()  
{
  Serial.begin(115200); //start serial communicatino at 115200 baud rate
  delay(5000);
  Serial.println("Adafruit GPS/RockBLOCK beacon");
    
  nss.begin(19200); //start rockblock serial
  isbd.attachConsole(Serial); //get Rockblock updates through serial port
  isbd.setPowerProfile(0); // set rockblock to using direct power
    
  GPS.begin(9600); //start GPS serial
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA); //set GPS data type
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_100_MILLIHERTZ);   // 0.1 Hz GPS update rate
  mySerial.println(PMTK_Q_RELEASE); //gives firmware version

}
uint32_t looptime = 600000;// minimum time between transmissions (milliseconds)
char gpsbuf[100]; //buffer which will be sent to rockblock
uint32_t timer = millis(); //timer for GPS parsing
void loop()                     // run over and over again
{
  uint32_t loopstarttime = millis(); //to keep track of how long has elapsed at end of loop
  boolean gpsfix = false; //when a fix is found this becomes true and triggers rockblock
  
  mySerial.listen(); //turns on comms with GPS  
  char c = GPS.read(); //check for new GPS data
  if ((c) && (GPSECHO)) //will print raw gps string to serial monitor
    Serial.write(c); 
  if (GPS.newNMEAreceived()) {
    if (!GPS.parse(GPS.lastNMEA()))   // this also sets the newNMEAreceived() flag to false
      return;  // we can fail to parse a sentence in which case we should just wait for another
  }
  if (timer > millis())  timer = millis(); //reset timer if millis() loops back to zero
  if (millis() - timer > 10000) { //only write new string every 10 seconds
    timer = millis(); // reset the timer
    
    String hourstr = String(GPS.hour); //these conversions all change number variables to string objects
    String minutestr = String(GPS.minute);
    String secondstr = String(GPS.seconds);
    String timestr = "Time:" + hourstr + ":" + minutestr + ":" + secondstr; //concatenate strings
    String daystr = String(GPS.day);
    String monthstr = String(GPS.month);
    String yearstr = String(GPS.year);  
    String datestr = "Date:" + daystr + "/" + monthstr + "/" + yearstr;
    String fixstr = String((int)GPS.fix);
    String qualstr = String((int)GPS.fixquality);
    String fqstr = "F:" + fixstr + " Q:" + qualstr;
    if (GPS.fix) //the data in this loop will only be available if there's a GPS fix
    {
      String latstr = String(GPS.latitude, 4);
      String lonstr = String(GPS.longitude, 4);
      String locstr = "Location:" + latstr + GPS.lat + "," + lonstr + GPS.lon;
      String spdstr = String(GPS.speed);
      String angstr = String(GPS.angle);
      String altstr = String(GPS.altitude);
      String satstr = String((int)GPS.satellites);
      String othstr = "S:" + spdstr + " A:" + angstr + " H:" + altstr + " S:" + satstr;

      gpsfix = true; //boolean to trigger rockblock transmission process
      String gpsstr = fqstr + " " + timestr + " " + datestr + " " + locstr + " " + othstr; //concatenate all strings
      Serial.println("");
      Serial.println(gpsstr); //display the gps string on serial monitor
      gpsstr.toCharArray(gpsbuf,100); //convert the string object to a character array that can be send by rockblock
    }
  }
  if (gpsfix) //if new GPS data is parsed and available this loop will run, sending a rockblock transmission
  {
    int signalQuality = -1; //for storing rockblock signal quality
    nss.listen(); //listen to the rockblock serial ports
    isbd.begin(); //start the rockblock

    int err = isbd.getSignalQuality(signalQuality); //check the signal quality
    if (err != 0) //if there's a rockblock error
    {
      Serial.print("SignalQuality failed: error ");
      Serial.println(err);
      return; //start the loop again
    }
  
    Serial.print("Signal quality is ");
    Serial.println(signalQuality);
  
    err = isbd.sendSBDText(gpsbuf); //send the buffer of gps data via the rockblock
    if (err != 0)
    {
      Serial.print("sendSBDText failed: error ");
      Serial.println(err);
      return; //restart the loop if rockblock hits error
    }
  
    Serial.println("Hey, it worked!");
    Serial.print("Messages left: ");
    Serial.println(isbd.getWaitingMessageCount()); //messages to be received by the rockblock
    isbd.sleep(); //put the rockblock to sleep
    delay(10000);
    uint32_t wait = looptime - millis() + loopstarttime; 
    if (millis()-loopstarttime < looptime)
      delay(wait); //wait until 10 minutes has elapsed since start of loop
  }
}
