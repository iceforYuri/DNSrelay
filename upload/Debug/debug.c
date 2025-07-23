#include "debug.h"

void debug_print1(const char *fmt, ...)
{
    if (debug_mode == 1 || debug_mode == 2)
    {
        va_list args;
        va_start(args, fmt);
        printf("[Thread %6lu] ", my_get_thread_id());
        vprintf(fmt, args);
        va_end(args);
    }
}

void debug_print2(const char *fmt, ...)
{
    if (debug_mode == 2)
    {
        va_list args;
        va_start(args, fmt);
        printf("[Thread %6lu] ", my_get_thread_id());
        vprintf(fmt, args);
        va_end(args);
    }
}

void debug_dns_message_hex(const unsigned char *buf, int len)
{
    if (debug_mode == 2)
    {
        printf("[Thread %6lu] ", my_get_thread_id());
        for (int i = 0; i < len; i++)
        {
            printf("%02X ", buf[i]);
        }
        printf("\n");
    }
}

void debug_print2_normal(const char *fmt, ...)
{
    if (debug_mode == 2)
    {
        va_list args;
        va_start(args, fmt);
        vprintf(fmt, args);
        va_end(args);
    }
}