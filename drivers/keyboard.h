#ifndef KEYBOARD_H
#define KEYBOARD_H
void keyboard_init();
extern volatile char last_key;
extern volatile int key_waiting;
#endif
