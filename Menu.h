#ifndef MENU_H
#define MENU_H

#include <Arduino.h>

/* The menu stores
  - Text of menu items
  - Measured or configured value for each menu item
  - Max, Min value allowed
  - Unit of measure

  The value primarily stores real numbers represented by int by 1/100th steps. But also special values as defined below. 
*/

enum MenuValues {
    OFF = -5000,
    ON,
    AUTO,
    ERROR,
    BLANK,
    SPECIALVALUE
};

#define DEFAULT_ROOM1_TEMP 2200      // 22 st. celsia
#define DEFAULT_ROOM2_TEMP 500       // 5 st. celsia - non freeze 
#define DEFAULT_MAX_WATER_TEMP 8500  // 85 st. celsia
#define DEFAULT_HEATER AUTO          // termostat
#define DEFAULT_PUMP AUTO            // cerpadlo
#define DEFAULT_HYSTEREZE 50         // half degree
#define TEMPSTEP 10                  // change temperature by 1/10 of degree

typedef struct item {
    String text; 
    int v;  // value for each menu item
    int v_increment; // value increment
    int v_max; // max value
    int v_min; // min value
    String v_unit; // unit, % of humidity or degrees of celsius
  } item_t;

class MyMenu {
  private:
    int index;
    int max_index;
    item_t *items;

  public:
    MyMenu (int no_menu_items, item_t *i);
    void beginning();   // sets menu to first item
    void next();        // move to next menu item
    void prev();        // move to previous menu item
    bool last();        // Is last menu item (Back or Select) selected?
    void value_increase();  // increase the value shown
    void value_decrease();  // decrease the value shown
    void value_set(int i, int v);  // sets the value, if out of range, sets defaults
    int value_get(int i);        // get the value
    int get_min(int i);
    int get_max(int i);
    String value_getastext();  // get text representation of the value
    String text_get();      // get menu item text
};



#endif
