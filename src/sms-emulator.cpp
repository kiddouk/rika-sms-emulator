#include "Arduino.h"
 
 
#define baudUSB 115200
#define baudRIKA 115200
 
#define EMPTY ""
#define ECHO "E"
#define FACTORY_RESET "&F"
#define IPR "+IPR"
#define DUMP "+DUMP"
#define READ_SMS "+CMGR"
#define SEND_SMS "+CMGS"
#define DELETE_SMS "+CMGD"
#define NEW_MESSAGE_INDICATION "+CNMI"
 
#define RIKA_COM Serial1
 
#include <EEPROM.h>
 
 
String at_command;
bool at_command_ready = false;
int eeStoreAddress = 0;
 
 
typedef enum AT_COMMAND {
                         UNKNOWN,
                         AT,
                         AT_FACTORY_SETTINGS,
                         AT_ECHO,
                         AT_IPR,
                         AT_READ_SMS,
                         AT_SEND_SMS,
                         AT_DELETE_SMS,
                         AT_NEW_MESSAGE_INDICATION
} AT_COMMAND_t;
 
// Process Command:
// Takes a string containing hopefully an AT command, and finds it equivalent in
// the command enum that we have defined.
// Also remove the "AT" part of the commands (ie: the first 2 bytes of the string).
AT_COMMAND_t process_at_command() {
 
  // We remove the "AT" part of the AT command (ex: AT+IPR)
  at_command.remove(0, 2);
  if (at_command.equals(EMPTY)) {
    return AT;
  }
 
  if (at_command.startsWith(IPR)) {
    return AT_IPR;
  }
 
  if (at_command.startsWith(READ_SMS)) {
    return AT_READ_SMS;
  }
 
  if (at_command.startsWith(DELETE_SMS)) {
    return AT_DELETE_SMS;
  }

  if (at_command.startsWith(FACTORY_RESET)) {
    return AT_FACTORY_SETTINGS;
  }

  if (at_command.startsWith(ECHO)) {
    return AT_ECHO;
  }

  if (at_command.startsWith(SEND_SMS)) {
    return AT_SEND_SMS;
  }

  if (at_command.startsWith(NEW_MESSAGE_INDICATION)) {
    return AT_NEW_MESSAGE_INDICATION;
  }
 
  return UNKNOWN;
}
 
void setup() {
 
  // put your setup code here, to run once:
  Serial.begin(baudUSB);
  RIKA_COM.begin(baudRIKA);
  Serial.println("Starting...");
 
  // Alloc enough space for a long AT command. Who knows... Rika can overflow us here ;).
  at_command.reserve(100);
}
 
/* &F E0
 *  CMGF=1 // Text mode for sms
 *  CNMI=0,0,0,0,1 // Configure Send/Receive stuff
 *  CMGD=1  // Discard MEssage 1
 *  CGMD=2  // Discard MEssage 2
 *  CMGD=3 // Dicard Message 3
 *  CMGR=1 // Read message 1
 *  CMGR=1 // Read message 1
 *  CMGD=1 // Delete message 1
 *  CMGF=1 // MEssage format = text
 *  CMGS="+545202020" (xF)AILURE: code error AT+CMGR=1
 */
 
void serialEvent1() {
  char incomingByte;
 
  while (RIKA_COM.available()) {
    incomingByte = (char) RIKA_COM.read();
    at_command += incomingByte;    
 
    // If we meet <CR> from Rika, then we return the computed string right away. Time to do something.
    if (incomingByte == char(13)) {
      at_command_ready = true;
      at_command.trim();
      return;
    }
  }
}



String read_sms() {
  char incomingByte;
  String sms;

  sms.reserve(160);
  
  while(RIKA_COM.available()) {
    incomingByte = (char) RIKA_COM.read();
    sms += incomingByte;
    
    if (incomingByte == char(26)) {
      return sms;
    }
  }
}


void send_ok() {
  RIKA_COM.write(char(13));
  RIKA_COM.write(char(10));
  RIKA_COM.print("OK");
  RIKA_COM.write(char(13));
  RIKA_COM.write(char(10));
}
 
void clearAtCommand() {
  at_command = "";
  at_command_ready = false;
}

String format_status_sms() {
  return String("+CMGR: \"REC READ\",\"+4520202020\",,\"07/04/20,10:08:02+32\"\r\n1234 ?\r\n");
}

String format_send_sms_ok() {
  return String("+CMGS: 1");
}
 
void send_payload(String payload) {
  RIKA_COM.print(payload);
  send_ok();
}

void loop() {
  // Nothing to do? See you in a bit...
  String payload;
  
  if (at_command_ready == false) {
    delay(1000);
    return;
  }
  
  AT_COMMAND receivedCommand = process_at_command();
  Serial.println("Command received :" + at_command);
  Serial.println(receivedCommand, DEC);
   
  switch (receivedCommand) {
  case AT:
  case AT_FACTORY_SETTINGS:
  case AT_ECHO:
  case AT_IPR:
  case AT_DELETE_SMS:
    send_ok();
    break;
 
  case AT_READ_SMS:
    payload = format_status_sms();
    reply_with_payload(payload);
    break;
    
  case AT_SEND_SMS:
    String sms = read_sms();
    Serial.println("received sms:");
    Serial.println(sms);
    Serial.println("end sms");
    payload = format_send_sms_ok();
    reply_with_payload(payload);
    break;
    
  default:
    send_ok();
    break;
  }
   
  clearAtCommand();
}
