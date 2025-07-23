#include "IdConversion.h"

ID_conversion ID_list[ID_LIST_SIZE];
int list_size = 0;
free_id_queue free_queue;
uint16_t next_id = 1; // 从1开始分配ID

// =============================================================================
// ID管理函数实现
// =============================================================================

// 初始化空闲队列
void init_ID_list()
{
    memset(ID_list, 0, sizeof(ID_list));

    // 初始化空闲队列
    free_queue.front = 0;
    free_queue.rear = 0;
    free_queue.count = 0;

    next_id = 1;
    // 从1开始分配ID
    // // 将所有ID加入空闲队列
    // for (uint16_t i = 1; i < ID_LIST_SIZE; i++)
    // {
    //     free_queue.free_ids[free_queue.rear] = i;
    //     free_queue.rear = (free_queue.rear + 1) % ID_LIST_SIZE;
    //     free_queue.count++;
    // }

    debug_print1("ID list initialized with %d free IDs\n", free_queue.count);
}

// mutex
uint16_t set_ID(uint16_t client_ID, struct sockaddr_in client_addr)
{
    time_t current_time = time(NULL);

    my_lockMutex(ID_list_Mutex);

    // 先尝试从空闲队列获取
    if (free_queue.count > 0)
    {
        uint16_t free_id = free_queue.free_ids[free_queue.front];
        free_queue.front = (free_queue.front + 1) % ID_LIST_SIZE;
        free_queue.count--;

        ID_list[free_id].client_ID = client_ID;
        ID_list[free_id].server_ID = free_id;
        ID_list[free_id].client_addr = client_addr;
        ID_list[free_id].expire_time = current_time + ID_EXPIRE_TIME;

        my_unlockMutex(ID_list_Mutex);

        debug_print2("ID allocated from queue: client_ID=%d -> server_ID=%d\n", client_ID, free_id);

        return free_id;
    }

    // 空闲队列为空，查找过期ID
    for (int attempts = 0; attempts < ID_LIST_SIZE; attempts++)
    {
        uint16_t id = next_id;
        next_id = (next_id % (ID_LIST_SIZE - 1)) + 1; // 循环1到ID_LIST_SIZE-1

        if (ID_list[id].expire_time == 0 || ID_list[id].expire_time < current_time)
        {
            ID_list[id].client_ID = client_ID;
            ID_list[id].server_ID = id;
            ID_list[id].client_addr = client_addr;
            ID_list[id].expire_time = current_time + ID_EXPIRE_TIME;

            my_unlockMutex(ID_list_Mutex);
            return id;
        }
    }

    my_unlockMutex(ID_list_Mutex);
    debug_print1("Warning: ID pool exhausted!\n");
    return 0;
}

int get_client_info(uint16_t server_ID, struct sockaddr_in *client_addr, uint16_t *client_ID)
{
    if (server_ID >= ID_LIST_SIZE || !client_addr || !client_ID)
    {
        return 0;
    }

    time_t current_time = time(NULL);

    if (ID_list[server_ID].expire_time >= current_time)
    {
        *client_addr = ID_list[server_ID].client_addr;
        *client_ID = ID_list[server_ID].client_ID;

        debug_print2("ID resolved: server_ID=%d -> client_ID=%d\n", server_ID, *client_ID);

        return 1;
    }

    return 0; // ID已过期
}

// 释放ID时加回队列
int delete_ID(uint16_t server_ID)
{
    if (server_ID >= ID_LIST_SIZE)
    {
        return 0;
    }

    my_lockMutex(ID_list_Mutex);

    if (ID_list[server_ID].expire_time > 0)
    {
        memset(&ID_list[server_ID], 0, sizeof(ID_conversion));

        // 将释放的ID加回空闲队列
        if (free_queue.count < (ID_LIST_SIZE - 1))
        {
            free_queue.free_ids[free_queue.rear] = server_ID;
            free_queue.rear = (free_queue.rear + 1) % ID_LIST_SIZE;
            free_queue.count++;
        }

        my_unlockMutex(ID_list_Mutex);

        debug_print2("ID released to queue: server_ID=%d\n", server_ID);

        return 1;
    }

    my_unlockMutex(ID_list_Mutex);
    return 0;
}

void cleanup_expired_IDs()
{
    time_t current_time = time(NULL);
    int cleaned = 0;

    for (int i = 0; i < ID_LIST_SIZE; i++)
    {
        if (ID_list[i].expire_time > 0 && ID_list[i].expire_time < current_time)
        {
            my_lockMutex(ID_list_Mutex);
            memset(&ID_list[i], 0, sizeof(ID_conversion));
            cleaned++;
            my_unlockMutex(ID_list_Mutex);
        }
    }

    if (cleaned > 0)
    {
        debug_print2("Cleaned %d expired IDs\n", cleaned);
    }
}
