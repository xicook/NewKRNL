#ifndef SHELL_H
#define SHELL_H
void shell_init();
void shell_update();
void shell_handle_key(char c);
extern int shell_active;
#endif
