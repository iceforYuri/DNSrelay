#include "header.h"
#include <stdarg.h>
#include <stdio.h>
#include "platformThread.h"

extern int debug_mode;
extern int message_count;

void debug_print1(const char *fmt, ...);
void debug_print2(const char *fmt, ...);
void debug_dns_message_hex(const unsigned char *buf, int len);