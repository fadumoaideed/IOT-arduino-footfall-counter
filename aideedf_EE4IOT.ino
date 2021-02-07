#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <rgb_lcd.h>
#include <WiFiUdp.h>
#define myusername "fadum"
rgb_lcd LCD;
WiFiUDP UDP;
unsigned int localUDPport = 8888;  // local port to listen on
char incomingPacket[255];  // buffer for incoming packets
char  replyPacket[] = "Board's IP is .....";  // a reply string to send back
const char ssid[] = " ";
const char password[] = "";
const char domainname[] = "fadum";
ESP8266WebServer SERVER(80);

#define BUTTON 15
#define LEDPIN 15
int red = 0, green = 255, blue = 0;
const long TASK1L = 5; //check button every 5ms
const long TASK2L = 60000; // average footfall per minuite
const long TASK3L = 1000; //update wifi every second
const long TASK4L = 10000; //update device discovery every 10 seconds
const char SUID[] = "ENTRANCE";

/******************************************************** AdaFruit DEFINES **********************************************************
*
************************************************************************************************************************************/
//#define NOPUBLISH      // comment this out once publishing at less than 10 second intervals
#define ADASERVER     "io.adafruit.com"     // do not change this
#define ADAPORT       8883                  // do not change this 
#define ADAUSERNAME   "fadumoaideed"               // ADD YOUR username here between the qoutation marks
#define ADAKEY        " " // ADD YOUR Adafruit key here betwwen marks

/******************************** Global instances / variables***************************************
 *  
 ***************************************************************************************************/
WiFiClientSecure client;    // create a class instance for the MQTT server
// create an instance of the Adafruit MQTT class. This requires the client, server, portm username and
// the Adafruit key
Adafruit_MQTT_Client MQTT(&client, ADASERVER, ADAPORT, ADAUSERNAME, ADAKEY);


/******************************** Feeds *************************************************************
 *  
 ***************************************************************************************************/
//Feeds we publish to

// Setup a feed called FOOTFALL_RESET to subscibe to HIGH/LOW changes
Adafruit_MQTT_Subscribe FOOTFALL_RESET = Adafruit_MQTT_Subscribe(&MQTT, ADAUSERNAME "/feeds/footfall_reset");
Adafruit_MQTT_Publish FOOTFALL = Adafruit_MQTT_Publish(&MQTT, ADAUSERNAME "/feeds/footfall");
Adafruit_MQTT_Publish AVFOOTFALL = Adafruit_MQTT_Publish(&MQTT, ADAUSERNAME "/feeds/avfootfall");
Adafruit_MQTT_Publish SENSORUID = Adafruit_MQTT_Publish(&MQTT, ADAUSERNAME "/feeds/sensoruid");
static const char *fingerprint PROGMEM = " ";

/********************************** Main*************************************************************
 *  
 ***************************************************************************************************/


int timespressed = 0,current,last,total = 0;   // counter for the number of button presses
bool btnState;
float averageRate;

  int checkbutton(void); // prototype the checkbutton function
  int checkbutton(void)
  {
    pinMode(BUTTON, INPUT);
    current = digitalRead(BUTTON);
  
    if( current &&  !last) 
    {
       delay(5);
       
       if (current != 0) //if current is not equal to false button is pressed
       {
         last = current;
         btnState = HIGH;
         pinMode(LEDPIN, OUTPUT);
         return 1;
       }
      else 
      {
        last = current;
        btnState = LOW;
        pinMode(LEDPIN, OUTPUT);
        return 0;
      }
    }
    
    else
    {
      last = current;
      btnState = LOW;
      pinMode(LEDPIN, OUTPUT);
      return 0;
    }
  }
 
  void task1 (void);
  void task1 (void)
  {
     checkbutton();
     if(btnState == HIGH) 
     {
      timespressed++;
      total++;
      Serial.print("timepressed: ");
      Serial.println(timespressed);
      Serial.print("total: ");
      Serial.println(total);
     }
  }
  
void task2 (void);
void task2 (void)
  {
    LCD.setCursor(0,0);
    averageRate = total/60.0;
    LCD.print("Average: ");
    LCD.print(averageRate);
    Serial.print("averageRate:");
    Serial.println(averageRate);
  }

void task3 (void);// if wifi is not connected then the led on 
void task3 (void)
{
  if(WiFi.status() != WL_CONNECTED) 
      { 
        delay(500);
        digitalWrite(LEDPIN,HIGH);
      }
      else
      {
        digitalWrite(LEDPIN,LOW);
      }
    LCD.setCursor(0,1);
    LCD.print(WiFi.localIP());
}

void respond (void);
void servepage (void);

void setup() 
{
  LCD.begin(16,2);
  Serial.begin(115200);  // put your setup code here, to run once:
  Serial.print("Attempting to connect to: ");
  Serial.print(ssid); 
  
  WiFi.begin(ssid,password);
  while(WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    Serial.print(".");
  }
  Serial.print("\n");
  Serial.println("Successfully connected");
  
  Serial.print("MAC address: ");
  Serial.println(WiFi.macAddress());
  Serial.print("Local IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("Gateway: ");
  Serial.println(WiFi.gatewayIP());
  Serial.print("Subnet: ");
  Serial.println(WiFi.subnetMask());
  Serial.print("dnsIP: ");
  Serial.println(WiFi.dnsIP());
  
  if(MDNS.begin(domainname))
  {
    Serial.print("Access your sever at https://");
    Serial.print("fadum");
    Serial.println(".local");
  }
   else
  {
    Serial.println("Error setting up MDNS responder!");
  } 
  
  SERVER.on("/", respond);
  SERVER.begin();
  Serial.println("...server is now running");
  
  if(UDP.begin(localUDPport)); 
  {
    Serial.print("Successfully connected to socket!");
    Serial.printf("Now listening at IP %s, UDP port %d\n", WiFi.localIP().toString().c_str(), localUDPport);
  }

  MQTT.subscribe(&FOOTFALL_RESET);  // subscribe to the FOOTFALL feed  
  client.setFingerprint(fingerprint); //secures mqtt
 }
 
void MQTTSubscribe (void); 
void UDP_ (void); 
unsigned long TASK1LC = 0, TASK2LC = 0, TASK3LC = 0, TASK4LC = 0;
long interval = 7000; // time in which the mqtt updates variables
long previousMillis = 0;

void loop() {
  
  #ifdef NOPUBLISH
  if ( !MQTT.ping() ) 
  {
    MQTT.disconnect();
  }
  #endif
  SERVER.handleClient();
  MDNS.update();
   
   unsigned long current_millis = millis();    
   
   if( (current_millis - TASK1LC) >= TASK1L )
   {
    TASK1LC = current_millis;
    task1();
   }
   
   if( (current_millis - TASK2LC) >= TASK2L)
   {
    TASK2LC = current_millis;
    task2();
    total = 0;
   }

   if( (current_millis - TASK3LC) >= TASK3L)
   {
    TASK3LC = current_millis;
    task3();
   }
   
   if( (current_millis - TASK4LC) >= TASK4L)
   {
    TASK4LC = current_millis;
    UDP_();
   }
 
 MQTTconnect();
 MQTTSubscribe();

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis > interval)
  {
        FOOTFALL.publish(timespressed); //only print in time interval and doesnt cause throttling issues
        AVFOOTFALL.publish(averageRate);//
        SENSORUID.publish(SUID);
        previousMillis = currentMillis;
  }
  
       
  // This function call ensures that we have a connection to the Adafruit MQTT server. This will make
  // the first connection and automatically tries to reconnect if disconnected. However, if there are
  // three consecutive failled attempts, the code will deliberately reset the microcontroller via the 
  // watch dog timer via a forever loop.
 
}

void respond (void){

  servepage();
}
void servepage ( void )
{
  String reply;
  if (SERVER.hasArg("RESET"))
  {
    timespressed = 0;
    total = 0;
  }
    
    reply += "<!DOCTYPE HTML>";
    reply += "<meta http-equiv=\"refresh\" content=\"2\">";
    reply += "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
    reply += "<html>";
    reply += "<head>";
    reply += "<title> Fadumo Aideed's Board</title>";
    reply += "<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
    reply += "</style>\n";
    reply += "</head>";
    reply += "<body>";
    reply += "<h1>This is Fadumo Aideed IOT Coursework</h1>";
    
    reply += "<br>";
    reply += "<h1> TOTAL FOOTFALL: ";
    reply += timespressed;
    reply += "</h1>";
    reply += "</br>";
    
    reply += "<h1> AVERAGE FOOTFALL: ";
    reply += averageRate;
    reply += "</h1>";
    
    reply += "<h1> Sensors Unique Identifier: ";
    reply += SUID;
    reply += "</h1>";
    
    reply += "<h1><a href=""?RESET"">RESET FOOTFALL</a>";
    reply += "</body>";
    reply += "</html>";
    
    SERVER.send(200, "text/html", reply);
                                     // end of HTML
}

void UDP_ (void)
{

  int packetSize = UDP.parsePacket();
  if (packetSize)
  {
    // receive incoming UDP packets
    Serial.printf("Received %d bytes from %s , port %d ", packetSize, UDP.remoteIP().toString().c_str(), UDP.remotePort());
    int len = UDP.read(incomingPacket, 255);
    if (len > 0)
    {
      incomingPacket[len] = 0;
    }
    Serial.printf("UDP packet contents: %s\n", incomingPacket);

    // send back a reply, to the IP address and port we got the packet from
    UDP.beginPacket(UDP.remoteIP(), UDP.remotePort());
    UDP.write(replyPacket);
    UDP.endPacket();
  }

}
/******************************* MQTT connect *******************************************************
 *  
 ***************************************************************************************************/

void MQTTconnect ( void ) 
{
  unsigned char tries = 0;

  // Stop if already connected.
  if ( MQTT.connected() ) 
  {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  while ( MQTT.connect() != 0 )                                        // while we are 
  {     
       Serial.println("Will try to connect again in five seconds");   // inform user
       MQTT.disconnect();                                             // disconnect
       delay(5000);                                                   // wait 5 seconds
       tries++;
       if (tries == 3) 
       {
          Serial.println("problem with communication, forcing WDT for reset");
          while (1)
          {
            ;   // forever do nothing
          }
       }
  }
   Serial.println("MQTT succesfully connected!");
}

void MQTTSubscribe (void)
{
   // an example of subscription code
  Adafruit_MQTT_Subscribe *subscription;                    // create a subscriber object instance
   if( current &&  !last)                                   //reduces delays in printing 
    {
       if (current != 0) 
       {
         subscription = MQTT.readSubscription(5000);       // Read a subscription and wait for max of 5 seconds.
         
          if (subscription == &FOOTFALL_RESET)                               // if the subscription we have receieved matches the one we are after
            {
              Serial.print("Recieved from the FOOTFALL subscription:");  // print the subscription out
              Serial.println((char *)FOOTFALL_RESET.lastread);                 // we have to cast the array of 8 bit values back to chars to print
        
                if (strcmp((char *)FOOTFALL_RESET.lastread,"HIGH") == 0)
              {
                timespressed = 0;
                Serial.printf("Footfall was reset!");
                
              }

            }
        }
     }   
}  
// Go to http://192.168.0.92/
