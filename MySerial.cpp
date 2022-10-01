// TBD poresit delku stringu pro kV pair po seriove lince.

#include <Arduino.h>
#include "MySerial.h"


MySerial::MySerial(unsigned long speed) {
  Serial.begin(speed);
}

void MySerial::SendKVPair (keyvalue_pair_t kv) {
  char str[KEYMAXLENGTH + 4 + 5 + 1]; // Key lentgh + 4 chars ('<','>',' ',',') + 5 digits (max length of the int is 99999) + NULL 
  
  if (kv.value < 99999) {
    sprintf(str, "<%s, %d>", kv.key, kv.value);
    Serial.print(str);
  }
}

bool MySerial::ReceiveKVPair(keyvalue_pair_t &kv) {
  bool newData = recvWithStartEndMarkers();
  bool ret = false;

  if (newData == true) {
      strcpy(tempChars, receivedChars);    
      kv = parseKeyValuePair();
      newData = false;
      ret = true;
  }
  return ret;
}

bool MySerial::recvWithStartEndMarkers() {
    bool newData = false;
    static boolean recvInProgress = false;
    static byte indx = 0;
    char startMarker = '<';
    char endMarker = '>';
    char rc;

    while (Serial.available() > 0 && newData == false) {
        rc = Serial.read();

        if (recvInProgress == true) {
            if (rc != endMarker) {
      
                receivedChars[indx] = rc;
                indx++;
                if (indx >= NUMCHARS) {
                    indx = NUMCHARS - 1;
                }
            }
            else {
                receivedChars[indx] = '\0'; // terminate the string
                recvInProgress = false;
                indx = 0;
                newData = true;
            }
        }

        else if (rc == startMarker) {
            recvInProgress = true;
        }
    }
    return newData;
}

keyvalue_pair_t MySerial::parseKeyValuePair() {      // split the data into its parts
    char *strtokIndx; // this is used by strtok() as an index
    keyvalue_pair_t kv;

    strtokIndx = strtok(tempChars,",");      // get the first part - the tag
    if (strlen(strtokIndx) <= KEYMAXLENGTH) {
      strcpy(kv.key, strtokIndx); // copy it to KEY 
    }
 
    strtokIndx = strtok(NULL, ","); // this continues where the previous call left off
    kv.value = atoi(strtokIndx);     // convert this part to an integer

    return kv;
}

