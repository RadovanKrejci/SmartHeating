#ifndef MYSERIAL_H
#define MYSERIAL_H

#define KEYMAXLENGTH 5

struct keyvalue_pair_t {
  char    key[KEYMAXLENGTH];
  int     value;

  keyvalue_pair_t() {
    strcpy(key,"");
    value = 0;
  }
  
  keyvalue_pair_t(char k[KEYMAXLENGTH], int v) {
    if (strlen(k) <= KEYMAXLENGTH) {
      strcpy(key,k);
      value = v;      
    }
  }
};

#define NUMCHARS 32 // buffer size for serial reader

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