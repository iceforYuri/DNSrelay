#ifndef DNS_MESSAGE_H
#define DNS_MESSAGE_H

#include "header.h" // For standard headers and types
#include "debug.h"  // For debugging macros

extern int debug_mode; // Debug mode variable, defined in main.c
extern int message_count;
// Forward declaration for DnsMessage, if needed by other structs early
// struct DnsMessage;

// Maximum length for a domain name in dot-separated format + null terminator
#define MAX_DOMAIN_NAME_LEN 256
// Maximum length for RDATA (can be adjusted, e.g. for TXT records)
#define MAX_RDATA_LEN 512
// Maximum number of RRs of a certain type (e.g. answers)
#define MAX_RRS 10
// Maximum size for general buffers

// DNS Class codes
// typedef enum
// {
//     QCLASS_IN = 1, // Internet
//     QCLASS_CS = 2, // CSNET
//     QCLASS_CH = 3, // CHAOS
//     QCLASS_HS = 4  // Hesiod
// } Qclass;

// Flag masks for header flag extraction
static const uint16_t QR_MASK = 0x8000;
static const uint16_t OPCODE_MASK = 0x7800;
static const uint16_t AA_MASK = 0x0400;
static const uint16_t TC_MASK = 0x0200;
static const uint16_t RD_MASK = 0x0100;
static const uint16_t RA_MASK = 0x0080;
static const uint16_t Z_MASK = 0x0070;
static const uint16_t RCODE_MASK = 0x000F;

// DNS Header Structure (RFC 1035, Section 4.1.1)
typedef struct
{
    uint16_t id; // Transaction ID

    // 将flags字段分解为各个标志位
    uint16_t qr : 1;     // Query/Response flag
    uint16_t opcode : 4; // Operation code
    uint16_t aa : 1;     // Authoritative Answer
    uint16_t tc : 1;     // Truncation
    uint16_t rd : 1;     // Recursion Desired
    uint16_t ra : 1;     // Recursion Available
    uint16_t z : 3;      // Reserved (must be zero)
    uint16_t rcode : 4;  // Response code

    uint16_t qdcount; // question部分的资源记录数
    uint16_t ancount; // answer部分的资源记录数
    uint16_t nscount; // authority部分的资源记录数
    uint16_t arcount; // additional部分的资源记录数
} DnsHeader;

// DNS Question Structure (RFC 1035, Section 4.1.2)
typedef struct DnsQuestion
{
    char *qname;              // Domain name (null-terminated string, e.g., "www.example.com")
    uint16_t qtype;           // Type of the query (e.g., A, MX, CNAME)
    uint16_t qclass;          // Class of the query (usually IN for Internet)
    struct DnsQuestion *next; // For linked list support
} DnsQuestion;

// DNS Resource Record (RR) Structure (RFC 1035, Section 4.1.3)
typedef struct DnsResourceRecord
{
    char *name;          // Domain name (null-terminated string)
    uint16_t type;       // RR type code
    uint16_t class_code; // RR class code (usually IN)
    uint32_t ttl;        // Time To Live in seconds
    uint16_t rdlength;   // Length of the RDATA field

    // Union for different record types
    union
    {
        struct
        {
            uint8_t ip_addr[4];
        } a_record;

        struct
        {
            char *name;
        } cname_record;

        struct
        {
            char *mname;      // Master name
            char *rname;      // Responsible name
            uint32_t serial;  // Serial number
            uint32_t refresh; // Refresh interval
            uint32_t retry;   // Retry interval
            uint32_t expire;  // Expire time
            uint32_t minimum; // Minimum TTL
        } soa_record;

        unsigned char *raw_data; // For other types
    } rdata;

    struct DnsResourceRecord *next; // For linked list support
} DnsResourceRecord;

// DNS Message Structure (to hold all parts of a DNS message)
typedef struct
{
    DnsHeader *header;
    DnsQuestion *questions;         // Linked list of questions
    DnsResourceRecord *answers;     // Linked list of answer records
    DnsResourceRecord *authorities; // Linked list of authority records
    DnsResourceRecord *additionals; // Linked list of additional records
} DnsMessage;

// DNS Response Codes (RCODE) (RFC 1035, Section 4.1.1)
// typedef enum
// {
//     RCODE_NO_ERROR = 0,        // No error condition
//     RCODE_FORMAT_ERROR = 1,    // Format error - The name server was unable to interpret the query.
//     RCODE_SERVER_FAILURE = 2,  // Server failure - The name server was unable to process this query.
//     RCODE_NAME_ERROR = 3,      // Name Error - Meaningful only for responses from an authoritative name server.
//     RCODE_NOT_IMPLEMENTED = 4, // Not Implemented - The name server does not support the requested kind of query.
//     RCODE_REFUSED = 5          // Refused - The name server refuses to perform the specified operation.
// } Rcode;

// DNS Query Types (QTYPE) (subset, common types)
// See RFC 1035 Section 3.2.2 and IANA assignments
typedef enum
{
    QTYPE_A = 1,     // a host address
    QTYPE_NS = 2,    // an authoritative name server
    QTYPE_CNAME = 5, // the canonical name for an alias
    QTYPE_SOA = 6,   // marks the start of a zone of authority
    QTYPE_PTR = 12,  // a domain name pointer
    QTYPE_MX = 15,   // mail exchange
    QTYPE_TXT = 16,  // text strings
    QTYPE_AAAA = 28  // IPv6 address
    // Add more as needed
} Qtype;

// 修正函数声明
size_t get_bits(uint8_t **buffer, int bits); // 返回类型应该是uint16_t或uint32_t

void set_bits(uint8_t **buffer, int bits, uint32_t value); // 参数类型应该是uint32_t

void get_message(DnsMessage *msg, uint8_t *buffer, uint8_t *start);

uint8_t *set_message(DnsMessage *msg, uint8_t *buffer, uint8_t ip_addrs[][4], int ip_count, int is_authoritative);

uint8_t *get_header(DnsMessage *msg, uint8_t *buffer);

uint8_t *set_header(DnsMessage *msg, uint8_t *buffer, uint8_t *ip_addr, int is_authoritative);

uint8_t *get_question(DnsMessage *msg, uint8_t *buffer, uint8_t *start);

uint8_t *set_question(DnsMessage *msg, uint8_t *buffer);

uint8_t *get_answer(DnsMessage *msg, uint8_t *buffer, uint8_t *start);

uint8_t *set_answer(DnsMessage *msg, uint8_t *buffer, uint8_t *ip_addr);

uint8_t *get_authority(DnsMessage *msg, uint8_t *buffer, uint8_t *start);

uint8_t *get_additional(DnsMessage *msg, uint8_t *buffer, uint8_t *start);

uint8_t *get_domain(uint8_t *buffer, char *name, uint8_t *start);

uint8_t *set_domain(uint8_t *buffer, char *name);

void free_message(DnsMessage *msg);

#endif // DNS_MESSAGE_H