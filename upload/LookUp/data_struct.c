#include "data_struct.h"
#include <ctype.h>

// 全局变量定义
lru_node *hash_table[HASH_SIZE];
lru_node *lru_head = NULL;
lru_node *lru_tail = NULL;
int cache_size = 0;

// =============================================================================
// IP链表管理辅助函数
// =============================================================================

// 创建IP节点
ip_node *create_ip_node(uint8_t ip[4])
{
    ip_node *node = malloc(sizeof(ip_node));
    if (!node)
    {
        return NULL;
    }

    memcpy(node->ip, ip, 4);
    node->next = NULL;
    return node;
}

// 释放IP链表
void free_ip_list(ip_node *head)
{
    while (head)
    {
        ip_node *temp = head;
        head = head->next;
        free(temp);
    }
}

// 添加IP到链表（避免重复）
void add_ip_to_list(ip_node **head, uint8_t ip[4], uint32_t ttl)
{
    // 检查是否已存在
    ip_node *current = *head;
    while (current)
    {
        if (memcmp(current->ip, ip, 4) == 0)
        {
            time_t now = time(NULL);
            current->ttl = ttl + now; // 更新TTL
            return;                   // IP已存在，不重复添加
        }
        current = current->next;
    }

    // 创建新节点并添加到头部
    ip_node *new_node = create_ip_node(ip);
    if (new_node)
    {
        new_node->next = *head;
        *head = new_node;
    }
}

// 从IP链表获取所有IP地址
int get_ip_from_list(ip_node *head, uint8_t ip_addrs[][4], int max_ips)
{
    int count = 0;
    ip_node *current = head;

    while (current && count < max_ips)
    {
        time_t now = time(NULL);
        if (current->ttl != TTL_STATIC && (current->ttl == 0 || now >= current->ttl))
        {
            // 如果TTL为0或已过期，跳过此IP
            current = current->next;
            continue;
        }
        memcpy(ip_addrs[count], current->ip, 4);
        count++;
        current = current->next;
    }

    return count;
}

// =============================================================================
// 哈希函数和双向链表操作（内部函数）
// =============================================================================

// 域名哈希函数（DJB2算法）
// 优化的哈希函数 - 针对域名特性
static uint32_t hash_domain(const char *domain)
{
    uint32_t hash = 0;
    const char *p = domain;

    // 跳过 www. 前缀以提高命中率
    if (strncmp(domain, "www.", 4) == 0)
    {
        p += 4;
    }

    // 使用更快的哈希算法
    while (*p)
    {
        hash = hash * 33 + (*p++ | 0x20); // 直接小写转换
    }

    return hash & (HASH_SIZE - 1); // 假设 HASH_SIZE 是 2 的幂
}

// 从双向链表中移除节点
static void remove_from_lru(lru_node *node)
{
    if (node->prev)
        node->prev->next = node->next;
    if (node->next)
        node->next->prev = node->prev;
}

// 将节点添加到LRU链表头部
static void add_to_head(lru_node *node)
{
    node->next = lru_head->next;
    node->prev = lru_head;

    if (lru_head->next)
    {
        lru_head->next->prev = node;
    }
    lru_head->next = node;

    // 如果这是第一个真实节点，更新tail
    if (node->next == lru_tail)
    {
        lru_tail->prev = node;
    }
}

// 将节点移动到链表头部
static void move_to_head(lru_node *node)
{
    remove_from_lru(node);
    add_to_head(node);
}

// 从哈希表中移除节点
static void remove_from_hash(lru_node *node)
{
    uint32_t hash = hash_domain(node->domain);
    lru_node **hash_ptr = &hash_table[hash];

    while (*hash_ptr && *hash_ptr != node)
    {
        hash_ptr = &((*hash_ptr)->hash_next);
    }

    if (*hash_ptr)
    {
        *hash_ptr = node->hash_next;
    }
}

// =============================================================================
// 缓存管理函数实现
// =============================================================================

void init_cache()
{
    // 初始化哈希表
    my_lockMutex(hash_table_Mutex);
    memset(hash_table, 0, sizeof(hash_table));

    // 创建LRU链表的哨兵节点
    lru_head = malloc(sizeof(lru_node));
    lru_tail = malloc(sizeof(lru_node));

    if (!lru_head || !lru_tail)
    {
        debug_print1("Error: Failed to allocate memory for cache sentinels.\n");
        exit(1);
    }

    // 初始化哨兵节点
    memset(lru_head, 0, sizeof(lru_node));
    memset(lru_tail, 0, sizeof(lru_node));

    lru_head->next = lru_tail;
    lru_tail->prev = lru_head;

    cache_size = 0;
    my_unlockMutex(hash_table_Mutex);

    debug_print1("Enhanced LRU cache initialized with hash table (size: %d)\n", HASH_SIZE);
}

int is_cache_valid(lru_node *node)
{
    if (!node)
        return 0;

    // 静态记录永不过期
    if (node->ttl == TTL_STATIC)
        return 1;

    time_t now = time(NULL);
    return (now - node->timestamp) < node->ttl;
}

int query_cache(char *domain, uint8_t ip_addrs[][4], int max_ips, int *is_authoritative)
{
    if (!lru_head || !domain || !ip_addrs)
    {
        return 0;
    }

    // 计算哈希值
    uint32_t hash = hash_domain(domain);
    lru_node *node = hash_table[hash];

    // 先清缓存
    // cleanup_expired_cache();

    // 在哈希冲突链中查找
    while (node)
    {
        // lru_node *next_node = node->hash_next; // ✅ 先保存下一个节点

        if (strcmp(node->domain, domain) == 0)
        {
            if (!is_cache_valid(node))
            {
                my_lockMutex(hash_table_Mutex);
                remove_from_hash(node);
                remove_from_lru(node);
                free_ip_list(node->ip_list); // 释放IP链表
                free(node);

                cache_size--;
                my_unlockMutex(hash_table_Mutex);

                debug_print1("Cache expired: %s\n", domain);

                return 0;
            } // 找到有效记录，获取所有IP地址
            my_lockMutex(hash_table_Mutex);
            int ip_count = get_ip_from_list(node->ip_list, ip_addrs, max_ips);
            move_to_head(node);
            *is_authoritative = node->is_authoritative;
            my_unlockMutex(hash_table_Mutex);

            if (debug_mode == 2)
            {
                debug_print2("Cache hit: %s -> %d IP(s)\n", domain, ip_count);
                for (int i = 0; i < ip_count; i++)
                {
                    debug_print2("  IP %d: %d.%d.%d.%d\n", i + 1,
                                 ip_addrs[i][0], ip_addrs[i][1], ip_addrs[i][2], ip_addrs[i][3]);
                }
            }

            return ip_count;
        }
        // my_lockMutex(hash_table_Mutex);
        node = node->hash_next;
        // my_unlockMutex(hash_table_Mutex);
    }

    debug_print2("Cache miss: %s\n", domain);

    return 0;
}

void update_cache(uint8_t ip_addrs[][4], int ip_count, uint32_t *ttl, char *domain, int is_authoritative)
{
    if (!lru_head || !domain || !ip_addrs || ip_count <= 0)
    {
        return;
    }

    uint32_t hash = hash_domain(domain);
    lru_node *node = hash_table[hash];

    // 检查是否已存在
    while (node)
    {
        if (strcmp(node->domain, domain) == 0)
        {
            // 更新现有节点，替换整个IP链表
            my_lockMutex(hash_table_Mutex);

            // 释放旧的IP链表
            free_ip_list(node->ip_list);
            node->ip_list = NULL;
            node->ip_count = 0;

            // 添加所有新IP到链表
            for (int i = 0; i < ip_count; i++)
            {
                add_ip_to_list(&(node->ip_list), ip_addrs[i], ttl[i]);

                node->ip_count++;
            }
            node->timestamp = time(NULL);
            node->ttl = TTL_SIZE;
            node->is_authoritative = 0; // 更新权威性
            // move_to_head(node);
            my_unlockMutex(hash_table_Mutex);

            if (debug_mode == 2)
            {
                debug_print2("Cache updated (multi): %s -> %d IPs (authoritative: %s)\n",
                             domain, ip_count, is_authoritative ? "yes" : "no");
                for (int i = 0; i < ip_count; i++)
                {
                    debug_print2("  IP %d: %d.%d.%d.%d\n", i + 1,
                                 ip_addrs[i][0], ip_addrs[i][1], ip_addrs[i][2], ip_addrs[i][3]);
                }
            }
            return;
        }
        node = node->hash_next;
    }

    // 创建新节点
    lru_node *new_node = malloc(sizeof(lru_node));
    if (!new_node)
    {
        debug_print1("Error: Failed to allocate memory for cache node.\n");
        return;
    }

    // 设置节点数据
    new_node->ip_list = NULL;
    new_node->ip_count = 0;

    // 添加所有IP到链表
    for (int i = 0; i < ip_count; i++)
    {
        add_ip_to_list(&(new_node->ip_list), ip_addrs[i], ttl[i]);
        new_node->ip_count++;
    }
    strncpy(new_node->domain, domain, MAX_SIZE - 1);
    new_node->domain[MAX_SIZE - 1] = '\0';
    new_node->timestamp = time(NULL);
    new_node->ttl = TTL_SIZE;
    new_node->is_authoritative = 0; // 设置权威性
    new_node->prev = NULL;
    new_node->next = NULL;
    new_node->hash_next = NULL;

    // 插入哈希表和LRU链表
    my_lockMutex(hash_table_Mutex);
    new_node->hash_next = hash_table[hash];
    hash_table[hash] = new_node;
    add_to_head(new_node);
    cache_size++;
    my_unlockMutex(hash_table_Mutex);

    // 检查容量限制
    if (cache_size > MAX_CACHE)
    {
        cleanup_expired_cache();
        if (cache_size > MAX_CACHE)
        {
            my_lockMutex(hash_table_Mutex);
            delete_cache();
            my_unlockMutex(hash_table_Mutex);
        }
    }

    debug_print2("Cache added (multi): %s -> %d IPs (size: %d/%d) (authoritative: %s)\n",
                 domain, ip_count, cache_size, MAX_CACHE, is_authoritative ? "yes" : "no");
}

void delete_cache()
{
    if (!lru_tail || cache_size == 0)
    {
        return;
    }

    // 获取LRU尾部的真实节点（不是哨兵）
    lru_node *tail_node = lru_tail->prev;

    if (tail_node == lru_head)
    {
        // 缓存为空
        return;
    }
    while (tail_node->ttl == TTL_STATIC)
    {
        // 静态记录不删除
        tail_node = tail_node->prev; // 获取下一个节点
    }

    debug_print1("Cache evicted: %s\n", tail_node->domain);

    // 从哈希表中移除
    remove_from_hash(tail_node); // 从LRU链表中移除
    remove_from_lru(tail_node);

    // 释放IP链表和节点内存
    free_ip_list(tail_node->ip_list);
    free(tail_node);
    cache_size--;
}

void cleanup_expired_cache()
{
    if (!lru_tail || cache_size == 0)
    {
        return;
    }

    lru_node *current = lru_tail->prev;

    my_lockMutex(hash_table_Mutex);
    while (current != lru_head)
    {
        lru_node *prev_node = current->prev;

        if (!is_cache_valid(current))
        {

            debug_print1("Cleaning expired cache: %s\n", current->domain);

            remove_from_hash(current);
            remove_from_lru(current);
            free_ip_list(current->ip_list); // 释放IP链表
            free(current);
            cache_size--;
        }

        current = prev_node;
    }
    my_unlockMutex(hash_table_Mutex);
}

// 新增：添加静态记录函数（永不过期）
void add_static_record(uint8_t ip_addr[4], char *domain)
{
    if (!lru_head || !domain || !ip_addr)
    {
        return;
    }

    uint32_t hash = hash_domain(domain);
    lru_node *node = hash_table[hash];

    // 检查是否已存在
    while (node)
    {
        if (strcmp(node->domain, domain) == 0)
        {
            // 更新现有节点为静态记录，添加IP到链表
            my_lockMutex(hash_table_Mutex);
            add_ip_to_list(&(node->ip_list), ip_addr, TTL_STATIC); // 永不过期
            node->ip_count++;
            node->timestamp = time(NULL);
            node->ttl = TTL_STATIC;     // 静态记录永不过期
            node->is_authoritative = 1; // 静态记录总是权威的
            my_unlockMutex(hash_table_Mutex);

            debug_print2("Static record updated: %s -> %d.%d.%d.%d (total: %d IPs)\n",
                         domain, ip_addr[0], ip_addr[1], ip_addr[2], ip_addr[3], node->ip_count);

            return;
        }
        node = node->hash_next;
    }

    // 创建新的静态记录节点
    lru_node *new_node = malloc(sizeof(lru_node));
    if (!new_node)
    {
        debug_print1("Error: Failed to allocate memory for static record.\n");
        return;
    } // 设置节点数据
    new_node->ip_list = create_ip_node(ip_addr); // 创建IP链表
    new_node->ip_count = 1;
    strncpy(new_node->domain, domain, MAX_SIZE - 1);
    new_node->domain[MAX_SIZE - 1] = '\0';
    new_node->timestamp = time(NULL);
    new_node->ttl = TTL_STATIC;     // 静态记录永不过期
    new_node->is_authoritative = 1; // 静态记录总是权威的
    new_node->prev = NULL;
    new_node->next = NULL;
    new_node->hash_next = NULL;

    // 插入哈希表
    my_lockMutex(hash_table_Mutex);
    new_node->hash_next = hash_table[hash];
    hash_table[hash] = new_node;

    // 静态记录不参与LRU，但为了保持一致性，仍然加入链表
    add_to_head(new_node);
    cache_size++;
    my_unlockMutex(hash_table_Mutex);

    debug_print2("Static record added: %s -> %d.%d.%d.%d\n",
                 domain, ip_addr[0], ip_addr[1], ip_addr[2], ip_addr[3]);
}

// =============================================================================
// 辅助函数实现
// =============================================================================
void transfer_IP(uint8_t *this_IP, char *IP_addr)
{
    if (!this_IP || !IP_addr)
    {
        return;
    }

    int len = strlen(IP_addr);
    int tmp = 0;
    int IP_pos = 0;
    char *ptr = IP_addr;

    for (int i = 0; i <= len && IP_pos < 4; i++)
    {
        if (i == len || *ptr == '.')
        {
            this_IP[IP_pos++] = tmp;
            tmp = 0;
        }
        else if (*ptr >= '0' && *ptr <= '9')
        {
            tmp = tmp * 10 + (*ptr - '0');
        }

        if (i < len)
            ptr++;
    }
}

void print_cache_stats()
{
    printf("\n=== Enhanced Cache Statistics ===\n");
    printf("Cache size: %d/%d\n", cache_size, MAX_CACHE);

    // 统计哈希分布
    int used_buckets = 0;
    int max_chain_length = 0;
    int total_chain_length = 0;

    for (int i = 0; i < HASH_SIZE; i++)
    {
        if (hash_table[i])
        {
            used_buckets++;
            int chain_length = 0;
            lru_node *node = hash_table[i];
            while (node)
            {
                chain_length++;
                node = node->hash_next;
            }
            total_chain_length += chain_length;
            if (chain_length > max_chain_length)
            {
                max_chain_length = chain_length;
            }
        }
    }

    printf("Hash buckets used: %d/%d (%.1f%%)\n",
           used_buckets, HASH_SIZE, used_buckets * 100.0 / HASH_SIZE);
    printf("Max chain length: %d\n", max_chain_length);
    printf("Avg chain length: %.2f\n",
           used_buckets > 0 ? (double)total_chain_length / used_buckets : 0.0);

    // 显示最近使用的几个缓存项
    printf("\nRecent cache entries:\n");
    lru_node *node = lru_head->next;
    int count = 0;
    while (node != lru_tail && count < 5)
    {
        printf("  %s -> ", node->domain);
        if (node->ip_list)
        {
            ip_node *ip_current = node->ip_list;
            int ip_count = 0;
            while (ip_current && ip_count < 3) // 最多显示3个IP
            {
                if (ip_count > 0)
                    printf(", ");
                printf("%d.%d.%d.%d",
                       ip_current->ip[0], ip_current->ip[1],
                       ip_current->ip[2], ip_current->ip[3]);
                ip_current = ip_current->next;
                ip_count++;
            }
            if (ip_current)
                printf("..."); // 如果还有更多IP
        }
        else
        {
            printf("(no IPs)");
        }
        printf("\n");
        node = node->next;
        count++;
    }
    printf("=================================\n\n");
}