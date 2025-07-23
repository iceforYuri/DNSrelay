#ifndef DEFAULT_H
#define DEFAULT_H

// DNS协议相关常量
#define DNS_PORT 53
#define MAX_DNS_SIZE 512
#define DNS_HEADER_SIZE 12

// 缓存和数据结构相关常量
#define MAX_SIZE 128
#define MAX_NUM 65536
#define MAX_CACHE 65536
#define ID_LIST_SIZE 65536
#define ID_EXPIRE_TIME 30

// QR字段，查询报与响应报
#define QUERY_MESSAGE 0x00
#define RESPONSE_MESSAGE 0x80

// OPCODE字段，查询类型，标准查询，反向查询，服务器状态查询
#define STANDARD_QUERY 0

// DNS记录类型
#define RR_A 1
#define RR_NS 2
#define RR_CNAME 5
#define RR_SOA 6
#define RR_PTR 12
#define RR_MX 15
#define RR_TXT 16
#define RR_AAAA 28

// DNS类
#define QCLASS_IN 1

// DNS响应码，ROCDE字段
#define RCODE_NO_ERROR 0
#define RCODE_FORMAT_ERROR 1
#define RCODE_SERVER_FAILURE 2
#define RCODE_NAME_ERROR 3
#define RCODE_NOT_IMPLEMENTED 4
#define RCODE_REFUSED 5

// 路径定义
#define LOG_FILE_PATH "Log/log.txt"

// 调试模式
extern int debug_mode;

#endif // DEFAULT_H
