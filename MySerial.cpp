// TBD poresit delku stringu pro kV pair po seriove lince.
// Communication format (both directions) is
// 3 letters + INT formatted to 5 digits, with start and end marker, comma separator and space. RegExp: ^<[A-Za-z]{3}, \d{5}>$
// The three letter character codes and their use are
// TT1 in/out 
// TT2 in/out
// T1 out
// T2 out
// AF in/out
// TBD: Extend these to H1, H2 - humidity data and WT1, WT2 - water temperature 1 for water out of stove, 2 for water coming back from radiators into the electrical boiler

#include <Arduino.h>
#include "MySerial.h"
#include "Menu.h"


MySerial::MySerial(unsigned long speed) {
  Serial.begin(speed);
}

void MySerial::SendKVPair (keyvalue_pair_t kv) {
  char str[KEYMAXLENGTH + 4 + 5 + 1]; // Key lentgh + 4 chars ('<','>',' ',',') + 5 digits (max length of the int is 99999) + NULL 
  
  if (kv.value != ERROR) {
    sprintf(str, "<%s, %d>", kv.key, kv.value);
    Serial.print(str);
    delay(20); // to avoid buffer overflow on the receiving end (ESP device) on low baud rates.
  }
  else
  {
    sprintf(str, "<%s, %d>", kv.key, 0);
    Serial.print(str);
    delay(20);
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

