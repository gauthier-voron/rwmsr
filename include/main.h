#ifndef MAIN_H
#define MAIN_H


extern const char     *program;
extern const char     *module;
extern uint8_t         verbose;


void error(const char *format, ...);

void vlog(const char *format, ...);


#endif
