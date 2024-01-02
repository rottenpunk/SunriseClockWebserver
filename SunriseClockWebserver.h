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

#ifndef SUNRISECLOCKWEBSERVER_H 
#define SUNRISECLOCKWEBSERVER_H 

#define RESPONSE_DELAY_MS      50  // Delay each time we don't have a character from response
#define RESPONSE_TIMEOUT_MS  2000  // Total timeout in msecs waiting for response from command.

const char * const commands[] = {
    "s",                        // Set dim level manually. if nnn is missing, then not set. 
    "o",                        // Turn light fully on.                                     
    "f",                        // Turn light fully off.                                    
    "t",                        // Set current time: thh:mm:ss .                                        
    "a",                        // Set alarm time & turn alarm on: ahh:mm:ss, or turn alarm on/off with special value.
    "c",                        // Cancel alarm if it has been triggered.                   
    "w",                        // Set wake up time in nnnnn secs if default not desired.         
    "d",                        // Force alarm going off.                                   
    "q",                        // Query current time, alarm time, current dimmer setting,  
};

typedef enum _command_id
{
  COMMAND_ID_S,                 // Set dim level.
  COMMAND_ID_O,                 // Set light fully on.
  COMMAND_ID_F,                 // Set light fully off.
  COMMAND_ID_T,                 // Set the time.
  COMMAND_ID_A,                 // Set the alarm time.
  COMMAND_ID_C,                 // Cancel the alarm.
  COMMAND_ID_W,                 // Set wake up time.
  COMMAND_ID_D,                 // Force alarm going off.
  COMMAND_ID_Q,                 // Query time, alarm time, dimmer value, wake secs, time set, alarm set, alarm triggered.
} COMMAND_ID;

#define ERROR_TIMEOUT  100      // If we time out waiting for a response to a command sent to the dimmer.
#define ERROR_BLOCKED  101      // Trying to send more than one command at a time to the dimmer.

#define COMMAND_ALARM_ON  99999   // Because alarm time only goes up to 60*60*24=86400 secs, we can use as special code.
#define COMMAND_ALARM_OFF 99998

#define MAX_CMDLINE   50

// Serial command input buffer... 
typedef struct _serial_buffer {
    int index = 0;
    char buffer[ MAX_CMDLINE+1];
} SerialBuffer;


int sendCommand(COMMAND_ID command_id, uint32_t value);

#endif  // SUNRISECLOCKWEBSERVER_H 
