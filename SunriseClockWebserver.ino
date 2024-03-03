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
#include <WebSocketsServer.h>
#include <time.h>
#include <stdbool.h>
#include <coredecls.h>  // crc32()
#include <PolledTimeout.h>
#include <include/WiFiState.h>  // WiFiState structure details

#include "LittleFS.h"
#include "SunriseClockWebserver.h"
#include "webpage.h"
   
ADC_MODE(ADC_VCC);    // For the ESP.getVcc() to return VCC value into the chip.

#define DEBUG_LOG_TIME_INTERVAL_MINUTES 5
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

struct config 
{
    uint32_t brightness;
    uint32_t alarmTime;
    bool     alarmSet;
    uint32_t wakeTime;
    uint32_t maxBright;     // This is a scale from 1-255.
    uint32_t minBright;     // The minimum level where bulb first turns on.
} cfg;

#define DEFAULT_WAKETIME        (30*60)   // Default amount of time alarm takes to get fully bright (seconds)
#define DEFAULT_MINIMUM_BRIGHTNESS   10   // This is the level right before the lightbulb turns on.
#define DEFAULT_MAXIMUM_BRIGHTNESS  240   // This is an arbitrary max level where the bulb is max on.

// Set web server port number to 80
// WiFiServer server(80);
ESP8266WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

char         dev_name[20];
SerialBuffer msgBuffer;      // For messages comming from the dimmer.
void process_dimmer_message( SerialBuffer *msgBuffer );

void setup() {
  
    Serial.begin(115200);    // We will be talking to dimmer.

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
    
    WiFi.setAutoReconnect(true);
    
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
    webSocket.begin();
    webSocket.onEvent(webSocketEvent);

    delay(2000);  // Wait 2 seconds to let NTP get time.
    while (time(NULL) == 0) {
        Serial.println("Waiting for NTP time update");
        delay(1000);  // Wait a second until NTP has a chance to get time.
    }

    // Define the default values, if no config has previously been saved...
    cfg.wakeTime  = DEFAULT_WAKETIME;
    cfg.maxBright = DEFAULT_MAXIMUM_BRIGHTNESS;
    cfg.minBright = DEFAULT_MINIMUM_BRIGHTNESS;  

    LittleFS.begin();
    restoreConfig();

    {
        time_t t = time(NULL); 
        struct tm *local = localtime ( &t );
        Serial.println(asctime(local)); 
    }
    
    sendCommand(COMMAND_ID_F, 0);   // Make sure light is off to begin with.
    cfg.brightness = 0;
    sendCommand(COMMAND_ID_T, 0);   // syncronize time with dimmer.     
    sendCommand(COMMAND_ID_A, cfg.alarmTime);  // syncronize alarm time with dimmer.     
    sendCommand(COMMAND_ID_W, cfg.wakeTime);  // syncronize alarm time with dimmer.     
    if (cfg.alarmSet) {
        sendCommand(COMMAND_ID_A, COMMAND_ALARM_ON);             
    } else {
        sendCommand(COMMAND_ID_A, COMMAND_ALARM_OFF); 
    }
}



void loop()
{
    char c;
    
    MDNS.update();
    webSocket.loop(); 
    server.handleClient();

    // Check to see if dimmer is sending us a message...
    if ( Serial.available() ) {
        if ( read_serial_input( &msgBuffer, '@', &c ) ) {
            process_dimmer_message( &msgBuffer );
        } else {
            Serial.print(c);
        }
    }
    
#ifdef DEBUG_LOG_TIME_INTERVAL_MINUTES
    // Output time every so often.  
    {
        time_t        rawtime;       // Temporary to test time();
        struct tm *   timeinfo;      // Temporary to test time();
        char          buffer[80];    // Temporary to test time();
        static time_t last_time = 0; // Temporary to test time();
        static int    last_status = 0;
        int           status;
        time(&rawtime);
        status = WiFi.status();
        if ( (last_time < rawtime && (rawtime % (60 * DEBUG_LOG_TIME_INTERVAL_MINUTES)) == 0) ||
              status != last_status)  { 
            last_status = status;      
            last_time = rawtime;
            timeinfo = localtime (&rawtime);     
            strftime (buffer, sizeof(buffer), "%x-%I:%M%p", timeinfo);
            Serial.print(buffer);
            Serial.print(" wifi: ");
            Serial.print(status);
            Serial.print(" free: ");
            Serial.print(ESP.getFreeHeap());
            Serial.print(" MHz: ");
            Serial.print(ESP.getCpuFreqMHz());
            Serial.print(" VCC: ");
            Serial.println(ESP.getVcc());
        }
    }
#endif
}



void saveConfig()
{
    File file = LittleFS.open("/ConfigFile.txt", "w");       
    if( file ) {
        file.write((uint8_t*)&cfg, sizeof(cfg));
        file.close();
    }
}



void restoreConfig()
{
    File file = LittleFS.open("/ConfigFile.txt", "r");       
    if( file ) {
        file.read((uint8_t*)&cfg, sizeof(cfg));
        file.close();
    }
}


// There are a few messages that the dimmer will send us...  
void process_dimmer_message(SerialBuffer *msgBuffer )
{
    char buf[30];
    int value;
    
    // cmdBuffer starts with start character @...
    switch (msgBuffer->buffer[1]) { 
        case 's':
            // write the current brightness value. Helpful when alarm triggered and we are fading on...
            value = snprintf(buf, sizeof(buf), "#s%d", 
                    map(cfg.brightness, cfg.minBright, cfg.maxBright, 0, 100));
            webSocket.broadcastTXT(buf, value); 
            break;
        default:
            break;
    }
}



void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length)
{
    char buf[30];
    uint16_t unmappedBrightness;
    uint8_t hours;
    uint8_t minutes;
    
    if(type == WStype_TEXT){
        
        if(payload[0] == '#'){

            switch( payload[1] ) {
                case 's':
                    unmappedBrightness = (uint16_t) strtol((const char *) &payload[2], NULL, 10);
                    cfg.brightness = map(unmappedBrightness, 0, 100, cfg.minBright, cfg.maxBright);
                    sendCommand(COMMAND_ID_S, cfg.brightness);  // Tell dimmer to set brightness.
                    saveConfig();
                    break;
                case 'a': 
                    if (payload[2] == 'o') {
                        cfg.alarmSet = true;
                        sendCommand(COMMAND_ID_A, COMMAND_ALARM_ON);
                    } else if (payload[2] == 'f') {
                        cfg.alarmSet = false;
                        sendCommand(COMMAND_ID_A, COMMAND_ALARM_OFF);
                    } else {
                        // Alarm is hh:mm, so convert to seconds before sending...
                        hours    = (((char)payload[2] - '0') * 10) + ((char)payload[3] - '0');
                        minutes  = (((char)payload[5] - '0') * 10) + ((char)payload[6] - '0');
                        cfg.alarmTime = (hours * 60 * 60) + (minutes * 60);
                        sendCommand(COMMAND_ID_A, cfg.alarmTime); 
                        sendCommand(COMMAND_ID_A, COMMAND_ALARM_ON);
                        cfg.alarmSet = true;
                    }
                    saveConfig();
                    break;
                case 'w':
                    cfg.wakeTime = (uint16_t) strtol((const char *) &payload[2], NULL, 10);            
                    sendCommand(COMMAND_ID_W, cfg.wakeTime);  // Tell dimmer
                    saveConfig();
                    break;
                case '?':    // Send back complete status (brightness, alarmtime, waketime)...
                    int value;
                    // write the current brightness value...
                    value = snprintf(buf, sizeof(buf), "#s%d", 
                            map(cfg.brightness, cfg.minBright, cfg.maxBright, 0, 100));
                    webSocket.broadcastTXT(buf, value); 
                    // write the current alarmtime value ("hh:mm"...
                    hours = cfg.alarmTime / 3600;
                    minutes = (cfg.alarmTime - (hours * 3600)) / 60;
                    value = snprintf(buf, sizeof(buf), "#a%02d:%02d", hours, minutes);
                    webSocket.broadcastTXT(buf, value); 
                    // write if the alarm is set or not (what do we call this?)..
                    if (cfg.alarmSet) {
                        webSocket.broadcastTXT("#ao", 3);
                    } else {
                        webSocket.broadcastTXT("#af", 3);
                    }                    
                    // write the current wake time value...
                    value = snprintf(buf, sizeof(buf), "#w%d", cfg.wakeTime);
                    webSocket.broadcastTXT(buf, value); 
                    break;
                default:
                    Serial.print("Reieved # but invalid command: ");
                    Serial.println( payload[1] );
                    break; 
            }
        } else {
            Serial.print("We recieved: ");
            for(int i = 0; i < length; i++)
                Serial.print((char) payload[i]);
            Serial.println();
        }
    }
}

void handle_NotFound()
{
    server.send(404, "text/plain", "Not found");
}

void handleWebpage() 
{
 server.send(200, "text/html", webpageCode); 
}
