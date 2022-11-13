#ifndef MYSERIAL_H
#define MYSERIAL_H

#define KEYMAXLENGTH 3 // 3 characters max for key

class keyvalue_pair_t {
  public:  
    char    key[KEYMAXLENGTH + 1];
    int     value;

    keyvalue_pair_t() {
      strcpy(key,"");
      value = 0;
    }
  
    keyvalue_pair_t(char k[], int v) {
      if (strlen(k) <= KEYMAXLENGTH) {
        strcpy(key,k);
        value = v;      
      }
    }
};

#define NUMCHARS 64 // buffer size for serial reader

class MySerial {
  private:
    char receivedChars[NUMCHARS]; // input buffer for receiving from serial input
    char tempChars[NUMCHARS];  
    bool recvWithStartEndMarkers();
    keyvalue_pair_t parseKeyValuePair();

  public:
    MySerial (unsigned long speed);
    void SendKVPair (keyvalue_pair_t kv);
    bool ReceiveKVPair(keyvalue_pair_t &kv);
};

#endif