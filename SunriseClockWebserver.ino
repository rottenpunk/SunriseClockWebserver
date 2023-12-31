//-----------------------------------------------------------------------------
//  Sunrise Clock Webserver
//
//  This is the webserver for the Sunrise Clock Webserver. 
//
//  See: https://github.com/rottenpunk/SunriseClockWebserver
//  and  https://github.com/rottenpunk/SunriseClockDimmer
//
//  Original code by Rui Santos at https://randomnerdtutorials.com  
//-----------------------------------------------------------------------------
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         // https://github.com/tzapu/WiFiManager
#include <ESP8266mDNS.h>
#include <time.h>
#include <stdbool.h>
#include "SunriseClockWebserver.h"
#include "webpage.h"


#define DEFAULT_DEVICE_NAME  "SunriseClock"  // Use this appended with ".local" in web browser.
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


// Set web server port number to 80
// WiFiServer server(80);
ESP8266WebServer server(80);

char dev_name[20];

void setup() {
  
    Serial.begin(9600);    // We will be talking to dimmer.

    configTime( time_zones[TIMEZONE_LOSANGELES], PREFERRED_NTP_SERVER);
  
    // WiFiManager
    // Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wifiManager;
  
    // Uncomment and run it once, if you want to erase all the stored information
    //wifiManager.resetSettings();
  
    // set custom ip for portal
    //wifiManager.setAPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));

    // fetches ssid and pass from eeprom and tries to connect
    // if it does not connect it starts an access point with the 
    // device name and goes into a blocking loop awaiting configuration
    //sprintf(dev_name, "ESP%06X", ESP.getChipId()); 
    strcpy(dev_name, DEFAULT_DEVICE_NAME);
    wifiManager.autoConnect(dev_name);
    Serial.print("Device name: ");
    Serial.println(dev_name);
    // or use this for auto generated name ESP + ChipID
    //wifiManager.autoConnect();
    
    if (!MDNS.begin(dev_name)) {          // Start the mDNS responder for esp8266.local
        Serial.println("Error setting up MDNS responder!");
    } else {
        MDNS.addService("http", "tcp", 80);  // Add service to MDNS...
        Serial.println("mDNS responder started");
    }
    server.on("/", handleWebpage);
    server.onNotFound(handle_NotFound);
  
    server.begin();
    Serial.println("HTTP server started");
    
    while (time(NULL) == 0) {
        Serial.println("Waiting for NTP time update");
        delay(1000);  // Wait a second until NTP has a chance to get time.
    }

    sendCommand(COMMAND_ID_F, 0);   // Make sure light is off to begin with.
    sendCommand(COMMAND_ID_Q, 0);   // Debug.
    sendCommand(COMMAND_ID_T, 0);   // syncronize time with dimmer.   
    sendCommand(COMMAND_ID_A, 40221);   // set alarm to 11:10:21
    delay(1000);
    sendCommand(COMMAND_ID_S, 80);  // test setting on light at dim level 80.
    delay(1000);
    sendCommand(COMMAND_ID_S, 130); // test setting on light at dim level 130.
    delay(1000);
    sendCommand(COMMAND_ID_F, 0);   // Light off.
}

void loop()
{
    time_t        rawtime;       // Temporary to test time();
    struct tm *   timeinfo;      // Temporary to test time();
    char          buffer[80];    // Temporary to test time();
    static time_t last_time = 0; // Temporary to test time();

    MDNS.update();

    time(&rawtime);
    // Debug: Display every 10 minutes...
    if (last_time < rawtime && (rawtime % (60 * 10)) == 0) {   
        last_time = rawtime;
        timeinfo = localtime (&rawtime);     
        strftime (buffer, sizeof(buffer), "Now it's %I:%M%p", timeinfo);
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
