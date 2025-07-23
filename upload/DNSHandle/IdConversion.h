#ifndef ID_CONVERSION_H
#define ID_CONVERSION_H

#include "header.h"
#include "debug.h"
#include "platformThread.h"
#include "platformSocket.h"

/* ID转换结构体 */
typedef struct
{
    uint16_t client_ID;             // 客户端原始ID
    uint16_t server_ID;             // 分配给上游服务器的ID
    struct sockaddr_in client_addr; // 客户端地址
    time_t expire_time;             // 过期时间
} ID_conversion;

// 添加空闲ID队列
typedef struct
{
    uint16_t free_ids[ID_LIST_SIZE];
    int front;
    int rear;
    int count;
} free_id_queue;

extern free_id_queue free_queue;
extern uint16_t next_id;

extern ID_conversion ID_list[ID_LIST_SIZE];
extern int list_size;
extern my_mutex *ID_list_Mutex;

// ID管理
void init_ID_list();
uint16_t set_ID(uint16_t client_ID, struct sockaddr_in client_addr);
int get_client_info(uint16_t server_ID, struct sockaddr_in *client_addr, uint16_t *client_ID);
int delete_ID(uint16_t server_ID);
void cleanup_expired_IDs();

#endif