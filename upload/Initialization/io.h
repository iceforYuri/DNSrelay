#ifndef IO_H
#define IO_H

#include "data_struct.h"
#include <time.h>
#include "platformThread.h"

#define HOST_PATH "Initialization/dnsrelay.txt"
#define MAX_PATH_LEN 100

// 全局变量声明 - 保持与data_struct.h一致
extern char IPAddr[MAX_SIZE];
extern char domain[MAX_SIZE];
extern int debug_mode;
extern char host_file_path[MAX_PATH_LEN]; // 默认路径

extern my_mutex* log_Mutex;

// 函数声明
void read_host();
void get_host_info(FILE *ptr);
void write_log(char *domain, uint8_t *ip_addr);

// 辅助函数声明
void transfer_IP(uint8_t *ip_array, char *ip_string);
void ip_to_string(uint8_t *ip_array, char *ip_string);
// 注意：add_node函数在data_struct.h中已声明，这里不重复声明

#endif // IO_H
