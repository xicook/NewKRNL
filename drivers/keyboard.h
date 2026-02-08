#ifndef KEYBOARD_H
#define KEYBOARD_H
void keyboard_init();
void keyboard_set_layout(int layout); // 0 = US, 1 = BR
extern volatile char last_key;
extern volatile int key_waiting;
#endif
