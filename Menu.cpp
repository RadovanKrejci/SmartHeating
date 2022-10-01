#include "Menu.h"


MyMenu::MyMenu (int no_menu_items, item_t *i, const char * const mmet[], int m_size) {
      max_index = no_menu_items - 1;
      items = i;
      index = 0;
      for (int i = 0; i < m_size; i++) {
        items[i].text_ptr = (char *)pgm_read_word(&mmet[i]);
      }
    }

void MyMenu::beginning() {
  index = 0;
}

void MyMenu::next() {
  index++;
  if (index > max_index)
    index = max_index;
}

void MyMenu::prev() {
  index--;
  if (index < 0)
    index = 0;
}

bool MyMenu::last() {
  return (index == max_index);
};

void MyMenu::value_increase() {
  item_t *i = &items[index];

  i->v += i->v_increment;
  if (i->v > i->v_max)
    i->v = i->v_max; 
}

void MyMenu::value_decrease() {
  item_t *i = &items[index];

  i->v -= i->v_increment;
  if (i->v < i->v_min)
    i->v = i->v_min; 
}

void MyMenu::value_set(int i, int v) {
        items[i].v = v;
    }

int MyMenu::value_get(int i) {
  return items[i].v;
}

int MyMenu::get_min(int i) {
  return items[i].v_min;
}

int MyMenu::get_max(int i) {
  return items[i].v_max;
}

String MyMenu::value_getastext() {
// takes input value and returns a 16 charactets string formated by value type with the right units
// anything else than SPECIAL VALUE is divided by 100 and displayed as real number with 1 decimals. 
// value 0, OFF is displayed as menu value[0]
// value 1, ON is displayed as menu value[1]
// value 2, AUTO is displayed as manu value[2]
// other values are error, menu value[3]
  String output;
  int value = items[index].v;
  String unit = items[index].v_unit;
  
  const char *menu_value[] = {
    "Vypnuto         ",
    "Zapnuto         ",
    "Auto            ",
    "Chyba           ",
    "                "
  };

  if (value >= SPECIALVALUE) {
    output = String(float(value) / 100, 1);
    output += unit;
  } else if (value >= OFF && value < SPECIALVALUE)
    output = menu_value[value - OFF];

  for (int i = output.length(); i < 16; i++)
    output += ' ';

  return output;
}

String MyMenu::text_get() {
  char buffer[17]; // 16 chars + NULL character

  strcpy_P(buffer, items[index].text_ptr);
  String output(buffer);
  return output;
}
