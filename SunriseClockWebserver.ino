 /*********
  Rui Santos
  Complete project details at https://randomnerdtutorials.com  
*********/

#include <ESP8266WiFi.h>
//#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         // https://github.com/tzapu/WiFiManager
#include <ESP8266mDNS.h>
#include <time.h>
#include "webpage.h"

#define DEFAULT_DEVICE_NAME "SunriseClock"  // once mDNS works, use this appended with ".local" in web browser.

#define PREFERRED_NTP_SERVER "pool.ntp.org" 

#define TIMEZONE_LOSANGELES 0
#define TIMEZONE_PORTLAND   1
#define TIMEZONE_PHOENIX    2
#define TIMEZONE_KEYWEST    3

const char *time_zones[] = {
    "PST8PDT,M3.2.0,M11.1.0",   // TIMEZONE_LOSANGELES  
    "PST8PDT,M3.2.0,M11.1.0",   // TIMEZONE_PORTLAND
    "MST7",                     // TIMEZONE_PHOENIX  
    "EST5EDT,M3.2.0,M11.1.0",   // TIMEZONE_KEYWEST
};

time_t now;                     // Seconds since Epoch (1970) - UTC
tm tm;                          // Holds time information in a more convenient way


// Set web server port number to 80
// WiFiServer server(80);
ESP8266WebServer server(80);

// Variable to store the HTTP request
String header;

// Assign output variables to GPIO pins
uint8_t LED1pin = 0;
bool LED1status = LOW;
uint8_t LED2pin = 2;
bool LED2status = LOW;

char dev_name[20];


void setup() {
  
    Serial.begin(115200);

    pinMode(LED1pin, OUTPUT);
    pinMode(LED2pin, OUTPUT);

    configTime( time_zones[TIMEZONE_LOSANGELES], PREFERRED_NTP_SERVER);
  
    // WiFiManager
    // Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wifiManager;
  
    // Uncomment and run it once, if you want to erase all the stored information
    //wifiManager.resetSettings();
  
    // set custom ip for portal
    //wifiManager.setAPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));

    // fetches ssid and pass from eeprom and tries to connect
    // if it does not connect it starts an access point with the specified name
    // here  "AutoConnectAP"
    // and goes into a blocking loop awaiting configuration
    //sprintf(dev_name, "ESP%06X", ESP.getChipId()); 
    strcpy(dev_name, DEFAULT_DEVICE_NAME); 
    wifiManager.autoConnect(dev_name);
    Serial.print("Device name: ");
    Serial.println(dev_name);
    // or use this for auto generated name ESP + ChipID
    //wifiManager.autoConnect();
    
    if (!MDNS.begin(dev_name)) {          // Start the mDNS responder for esp8266.local
        Serial.println("Error setting up MDNS responder!");
    }
    Serial.println("mDNS responder started");

    server.on("/", handleWebpage);
    server.onNotFound(handle_NotFound);
  
    server.begin();
    Serial.println("HTTP server started");

    // Add service to MDNS...
    MDNS.addService("http", "tcp", 80);
 
}

void loop()
{
    time_t rawtime;         // Temporary to test time();
    struct tm * timeinfo;   // Temporary to test time();
    char buffer[80];        // Temporary to test time();
    static time_t last_time_display = 0; // Temporary to test time();

    MDNS.update();

    time(&rawtime);
    if( last_time_display < rawtime && (rawtime % 60) == 0 ) {     // Only display once a minute.
        last_time_display = rawtime;
        timeinfo = localtime (&rawtime);     
        strftime (buffer, sizeof(buffer), " Now it's %I:%M%p", timeinfo);
        Serial.println(buffer);
    }
  
    server.handleClient();
}


void handle_NotFound()
{
    server.send(404, "text/plain", "Not found");
}

void handleWebpage() 
{
 server.send(200, "text/html", webpageCode); 
}
