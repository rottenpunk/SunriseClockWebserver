//-----------------------------------------------------------------------------
// Manage communication link to send commands to dimmer and recieve a
// response back.
//-----------------------------------------------------------------------------
#include <cstdbool>
#include <cstdio>
#include <cstdlib>
#include "HardwareSerial.h"
#include "SunriseClockWebserver.h"

// Cannot send more than one command to dimmer at a time...
static bool waiting_for_response = false;  // Indicate when a command sent and waiting.

static SerialBuffer serialBuffer;          // Command buffer used for collecting response.

#define MAX_TEMP_BUFFER  50         // Size of a temp buffer for building part of cmd.     

//-----------------------------------------------------------------------------
// Read input from a serial port and build command line buffer. Return TRUE if
// we have a full command to process (received a CRLF - we'll ignore CRs)...
//-----------------------------------------------------------------------------
bool read_serial_input( SerialBuffer *serBuff )
{
    char c;

    c = Serial.read();
    
    //Serial.println((uint8_t)c, HEX);

    if (serBuff->index == 0 && c != '#') {
        // Ignore anything before the '#' (which could be a '>').
    } else if (c == '\r') {             // LF?
        // null terminate command line. 
        serBuff->buffer[serBuff->index] = 0;  
        serBuff->index = 0;      // reset command line index.
        return true;
    } else if (c == '\n') {      // CR? 
        // Ignore CRs...
    } else {                     // Anything else, add to cmd buff if not full...
        if (serBuff->index < MAX_CMDLINE - 1) {
            serBuff->buffer[serBuff->index++] = c;  
        }
    }

    return false;
}

//-----------------------------------------------------------------------------
// sendCommand() -- Send command to dimmer, wait for a complete response back.
// 
// Returns either the number response, or if error, a negative number indicat-
// the error code (negated)... 
//-----------------------------------------------------------------------------
int sendCommand(COMMAND_ID command_id, int value) 
{
    int         timeout = 0;     // Time out counter, counting up to RESPONSE_TIMEOUT_MS.
    time_t      rawtime;
    struct tm*  tinfo;
    char        temp_str[MAX_TEMP_BUFFER];
    int         rc;
    int         hours;
    
    if (!waiting_for_response) {
        //waiting_for_response = true;  // Indicate waiting for response.
        
        Serial.print("###");                   // Send command prefix.
        Serial.print(commands[command_id]);    // Send command.
        
        switch (command_id) {
            
            case COMMAND_ID_T:                 // Set the time.
                time (&rawtime);
                tinfo = localtime (&rawtime);
                snprintf(temp_str, sizeof(temp_str), "%02d:%02d:%02d", tinfo->tm_hour, tinfo->tm_min, tinfo->tm_sec);
                Serial.println(temp_str);
                break;
                
            case COMMAND_ID_A:                 // Set the alarm time or turn alarm on/off.
                // one way to do it: value is the number of seconds since midnight...
                hours = value / 3600;
                snprintf(temp_str, sizeof(temp_str), "%02d:%02d:%02d",
                    hours,
                    (value - (hours * 3600)) / 60,
                    value % 60 );
                Serial.println(temp_str);
                break;    
                                
            case COMMAND_ID_S:                 // Set dim level.
                snprintf(temp_str, sizeof(temp_str), "%03d", value);
                Serial.println(temp_str);
                break;
                
            case COMMAND_ID_W:                 // Set wake up time.    
                snprintf(temp_str, sizeof(temp_str), "%05d", value);
                Serial.println(temp_str);
                break;
                
            case COMMAND_ID_O:                 // Set light fully on.
            case COMMAND_ID_F:                 // Set light fully off.
            case COMMAND_ID_C:                 // Cancel the alarm.
            case COMMAND_ID_D:                 // Force alarm going off.
            case COMMAND_ID_Q:                 // Query time, alarm time, dimmer value, wake secs, time set, alarm set, alarm triggered.
            default:
                Serial.println();
                break;
        }
        
        while (1) {    // We will loop, waiting for complete response.
            delay( RESPONSE_DELAY_MS );
            timeout += RESPONSE_DELAY_MS;  // Increment timeout value;
            if ( Serial.available() && read_serial_input( &serialBuffer ) ) {
                break;
            }
            if (timeout > RESPONSE_TIMEOUT_MS) {
                waiting_for_response = false;
                Serial.println("Error: Response timeout");
                return -(ERROR_TIMEOUT);                        
            }
        }

        // Response should start with # followed by either E (error) or some number. In the case 
        // of the q command (query), there will be # followed by a string with various info in it.
        
        if (serialBuffer.buffer[0] == '#') {
            // Did we get an error?
            if (serialBuffer.buffer[1] == 'E') {
                rc = -(atoi( &(serialBuffer.buffer[2])));
            }
            // Otherwise, if not response from q command, return returned int value...
            if (command_id == COMMAND_ID_Q) {
                rc = 0;
            } else {
                rc = atoi( &(serialBuffer.buffer[1]));
            }
        }

        // Debug...
        Serial.print("received: ");
        Serial.println(serialBuffer.buffer);
        Serial.print("response: ");
        Serial.println(rc);

        waiting_for_response = false;  // Indicate not waiting for response anymore.
        return rc;
    }
    return -(ERROR_BLOCKED);
}