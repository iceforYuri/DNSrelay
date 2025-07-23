#pragma once

#include "header.h"
#include "IdConversion.h"
// #include "default.h"
// #include "system.h"
#include <time.h>
#include "platformThread.h"

// 新增哈希表大小定义
#define HASH_SIZE 1024
#define TTL_SIZE 300          // 默认TTL为300秒（5分钟）
#define TTL_STATIC 0xFFFFFFFF // 静态记录标识（永不过期）

// 全局变量声明
extern char IPAddr[MAX_SIZE];
extern char domain[MAX_SIZE];
extern my_mutex *hash_table_Mutex;

// IP地址链表节点
typedef struct ip_node
{
    uint8_t ip[4];        // IP地址
    uint32_t ttl;         // 生存时间
    struct ip_node *next; // 指向下一个IP
} ip_node;

/* 优化的LRU缓存结构体 - 双向链表 + 哈希表 + 多IP支持 */
typedef struct cache_node
{
    ip_node *ip_list;             // IP地址链表（支持多个IP）
    int ip_count;                 // IP地址数量
    char domain[MAX_SIZE];        // 域名
    time_t timestamp;             // 时间戳
    uint32_t ttl;                 // 生存时间
    int is_authoritative;         // 权威性标识
    struct cache_node *prev;      // 前驱节点
    struct cache_node *next;      // 后继节点
    struct cache_node *hash_next; // 哈希冲突链表
} lru_node;

// 全局变量声明
extern lru_node *hash_table[HASH_SIZE];
extern lru_node *lru_head;
extern lru_node *lru_tail;
extern int cache_size;

// 函数声明
// 缓存管理
void init_cache();
int query_cache(char *domain, uint8_t ip_addrs[][4], int max_ips, int *is_authoritative); // 修改：支持多个IP地址
// void update_cache(uint8_t ip_addr[4], char *domain);                        // 保持单IP更新接口
void update_cache(uint8_t ip_addrs[][4], int ip_count, uint32_t *ttl, char *domain, int is_authoritative); // 新增：多IP更新接口，包含权威性
void add_static_record(uint8_t ip_addr[4], char *domain);                                                  // 新增：添加静态记录
void delete_cache();
void cleanup_expired_cache();
int is_cache_valid(lru_node *node);

// 辅助函数
void transfer_IP(uint8_t *this_IP, char *IP_addr);
int get_num(uint8_t val);
void print_cache_stats();
