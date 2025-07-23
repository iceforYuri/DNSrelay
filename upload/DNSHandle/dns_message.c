#include "dns_message.h"

#ifdef _WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif

// 获取指定位数的数据并转换字节序
size_t get_bits(uint8_t **buffer, int bits)
{
    if (bits == 8)
    {
        uint8_t val = **buffer; // 读取一个字节
        (*buffer)++;            // 移动指针一位
        return val;
    }
    if (bits == 16)
    {
        uint16_t val;
        memcpy(&val, *buffer, 2); // 读取两个字节
        *buffer += 2;             // 移动指针两位
        return ntohs(val);
    }
    if (bits == 32)
    {
        uint32_t val;
        memcpy(&val, *buffer, 4);
        *buffer += 4;
        return ntohl(val);
    }
    return 0;
}

// 设置指定位数的数据并转换字节序
void set_bits(uint8_t **buffer, int bits, uint32_t value)
{
    if (bits == 8)
    {
        **buffer = (uint8_t)value;
        (*buffer)++;
    }
    else if (bits == 16)
    {
        uint16_t val = htons((uint16_t)value);
        memcpy(*buffer, &val, 2);
        *buffer += 2;
    }
    else if (bits == 32)
    {
        uint32_t val = htonl(value);
        memcpy(*buffer, &val, 4);
        *buffer += 4;
    }
}

void get_message(DnsMessage *msg, uint8_t *buffer, uint8_t *start)
{
    if (!msg || !buffer || !start)
    {
        return;
    }

    // 初始化指针
    msg->header = NULL;
    msg->questions = NULL;
    msg->answers = NULL;
    msg->authorities = NULL;
    msg->additionals = NULL;

    // 分配头部空间
    msg->header = malloc(sizeof(DnsHeader));
    if (!msg->header)
    {
        return;
    }

    // debug_print1("%d: *", message_count++);
    // 解析头部
    buffer = get_header(msg, buffer);

    // 解析问题部分
    if (msg->header->qdcount > 0)
    {
        buffer = get_question(msg, buffer, start);
    }

    // 解析答案部分
    if (msg->header->ancount > 0)
    {
        buffer = get_answer(msg, buffer, start);
    }

    // 解析权威部分（如果需要）
    if (msg->header->nscount > 0)
    {
        buffer = get_authority(msg, buffer, start);
    }

    // 解析附加部分（如果需要）
    if (msg->header->arcount > 0)
    {
        buffer = get_additional(msg, buffer, start);
    }
}

uint8_t *set_message(DnsMessage *msg, uint8_t *buffer, uint8_t ip_addrs[][4], int ip_count, int is_authoritative)
{
    // debug_print1("%d:  ", message_count++);
    buffer = set_header(msg, buffer, ip_addrs[0], is_authoritative); // 使用第一个IP作为标识这是响应
    buffer = set_question(msg, buffer);

    // 为每个IP地址创建答案记录
    for (int i = 0; i < ip_count; i++)
    {
        buffer = set_answer(msg, buffer, ip_addrs[i]);
    }

    // buffer = set_authority(msg, buffer);
    // buffer = set_additional(msg, buffer);
    return buffer;
}

// 解析DNS头部
uint8_t *get_header(DnsMessage *msg, uint8_t *buffer)
{
    if (!msg || !msg->header || !buffer)
    {
        return buffer;
    }

    uint8_t *ptr = buffer;

    // 解析ID
    msg->header->id = get_bits(&ptr, 16);

    // 解析flags字段
    uint16_t flags_val = get_bits(&ptr, 16);

    // 使用掩码提取各个标志位
    msg->header->qr = (flags_val & QR_MASK) >> 15;
    msg->header->opcode = (flags_val & OPCODE_MASK) >> 11;
    msg->header->aa = (flags_val & AA_MASK) >> 10;
    msg->header->tc = (flags_val & TC_MASK) >> 9;
    msg->header->rd = (flags_val & RD_MASK) >> 8;
    msg->header->ra = (flags_val & RA_MASK) >> 7;
    msg->header->z = (flags_val & Z_MASK) >> 4;
    msg->header->rcode = (flags_val & RCODE_MASK);

    // 解析计数字段
    msg->header->qdcount = get_bits(&ptr, 16);
    msg->header->ancount = get_bits(&ptr, 16);
    msg->header->nscount = get_bits(&ptr, 16);
    msg->header->arcount = get_bits(&ptr, 16);

    return ptr;
}

// 设置DNS头部到缓冲区
uint8_t *set_header(DnsMessage *msg, uint8_t *buffer, uint8_t *ip_addr, int is_authoritative)
{
    if (!msg || !msg->header || !buffer)
    {
        return buffer;
    }
    uint8_t *ptr = buffer;

    // 设置响应标志（如果这是一个响应）
    if (ip_addr)
    {
        msg->header->qr = 1;                // 设置为响应
        msg->header->aa = is_authoritative; // 权威应答
        msg->header->ra = 1;                // 支持递归

        // 检查IP地址是否为0.0.0.0，设置响应码
        if (ip_addr[0] == 0 && ip_addr[1] == 0 &&
            ip_addr[2] == 0 && ip_addr[3] == 0)
        {
            msg->header->rcode = RCODE_NAME_ERROR;
            // 对于错误响应，ancount应该为0
            msg->header->ancount = 0;
        }
        else
        {
            msg->header->rcode = RCODE_NO_ERROR;
            // 注意：ancount应该由调用者在调用set_header之前设置好
            // 不在这里强制修改ancount的值
        }
    }

    // 设置ID
    set_bits(&ptr, 16, msg->header->id);

    // 构建flags字段
    uint16_t flags = 0;
    flags |= (msg->header->qr << 15) & QR_MASK;
    flags |= (msg->header->opcode << 11) & OPCODE_MASK;
    flags |= (msg->header->aa << 10) & AA_MASK;
    flags |= (msg->header->tc << 9) & TC_MASK; // 限制最大512字节
    flags |= (msg->header->rd << 8) & RD_MASK;
    flags |= (msg->header->ra << 7) & RA_MASK;
    flags |= (msg->header->z << 4) & Z_MASK;
    flags |= (msg->header->rcode) & RCODE_MASK;

    set_bits(&ptr, 16, flags);

    // 设置计数字段
    set_bits(&ptr, 16, msg->header->qdcount);
    set_bits(&ptr, 16, msg->header->ancount);
    set_bits(&ptr, 16, msg->header->nscount);
    set_bits(&ptr, 16, msg->header->arcount);

    return ptr;
}

// 解析DNS问题部分
uint8_t *get_question(DnsMessage *msg, uint8_t *buffer, uint8_t *start)
{
    if (!msg || !buffer || !start)
    {
        return buffer;
    }

    uint8_t *ptr = buffer;

    for (int i = 0; i < msg->header->qdcount; i++)
    {
        char name[MAX_DOMAIN_NAME_LEN] = {0};
        DnsQuestion *question = malloc(sizeof(DnsQuestion));

        if (!question)
        {
            continue; // 内存分配失败，跳过
        }

        // 解析域名
        ptr = get_domain(ptr, name, start);

        // 分配并复制域名
        question->qname = malloc(strlen(name) + 1);
        if (question->qname)
        {
            strcpy(question->qname, name);
        }
        // debug_print1("%s, TYPE: %d, CLASS: %d\n", question->qname, question->qtype, question->qclass);

        // 解析类型和类
        question->qtype = get_bits(&ptr, 16);
        question->qclass = get_bits(&ptr, 16);

        // 添加到链表头部
        question->next = msg->questions;
        msg->questions = question;
    }

    return ptr;
}

// 设置DNS问题部分到缓冲区
uint8_t *set_question(DnsMessage *msg, uint8_t *buffer)
{
    if (!msg || !buffer)
    {
        return buffer;
    }

    uint8_t *ptr = buffer;
    DnsQuestion *question = msg->questions;

    for (int i = 0; i < msg->header->qdcount && question; i++)
    {
        // debug_print1("%s, TYPE: %d, CLASS: %d\n", question->qname, question->qtype, question->qclass);
        ptr = set_domain(ptr, question->qname);
        set_bits(&ptr, 16, question->qtype);
        set_bits(&ptr, 16, question->qclass);

        question = question->next;
    }

    return ptr;
}

// 解析DNS答案部分
uint8_t *get_answer(DnsMessage *msg, uint8_t *buffer, uint8_t *start)
{
    if (!msg || !buffer || !start)
    {
        return buffer;
    }

    uint8_t *ptr = buffer;

    for (int i = 0; i < msg->header->ancount; i++)
    {
        char name[MAX_DOMAIN_NAME_LEN] = {0};
        DnsResourceRecord *record = malloc(sizeof(DnsResourceRecord));

        if (!record)
        {
            continue; // 内存分配失败，跳过
        }

        // 解析域名
        ptr = get_domain(ptr, name, start);

        // 分配并复制域名
        record->name = malloc(strlen(name) + 1);
        if (record->name)
        {
            strcpy(record->name, name);
        }

        // 解析记录头部
        record->type = get_bits(&ptr, 16);
        record->class_code = get_bits(&ptr, 16);
        record->ttl = get_bits(&ptr, 32);
        record->rdlength = get_bits(&ptr, 16);

        // 根据类型解析RDATA
        if (record->type == QTYPE_A && record->rdlength == 4)
        {
            // IPv4地址
            for (int j = 0; j < 4; j++)
            {
                record->rdata.a_record.ip_addr[j] = get_bits(&ptr, 8);
            }
        }
        else if (record->type == QTYPE_CNAME)
        {
            // CNAME记录
            char cname[MAX_DOMAIN_NAME_LEN] = {0};
            get_domain(ptr, cname, start);
            record->rdata.cname_record.name = malloc(strlen(cname) + 1);
            if (record->rdata.cname_record.name)
            {
                strcpy(record->rdata.cname_record.name, cname);
            }
            ptr += record->rdlength;
        }
        else
        {
            // 其他类型，分配原始数据空间
            record->rdata.raw_data = malloc(record->rdlength);
            if (record->rdata.raw_data)
            {
                memcpy(record->rdata.raw_data, ptr, record->rdlength);
            }
            ptr += record->rdlength;
        }

        // 添加到链表头部
        record->next = msg->answers;
        msg->answers = record;
    }

    return ptr;
}

uint8_t *set_answer(DnsMessage *msg, uint8_t *buffer, uint8_t *ip_addr)
{
    if (!msg || !buffer || !ip_addr)
    {
        return buffer;
    }

    uint8_t *ptr = buffer;

    // 使用问题中的域名作为答案域名
    if (msg->questions && msg->questions->qname)
    {
        ptr = set_domain(ptr, msg->questions->qname);
    }

    set_bits(&ptr, 16, QTYPE_A);   // type: A记录
    set_bits(&ptr, 16, QCLASS_IN); // class: IN
    set_bits(&ptr, 32, 300);       // TTL: 300秒
    set_bits(&ptr, 16, 4);         // rdlength: 4字节

    // 设置IP地址
    for (int i = 0; i < 4; i++)
    {
        set_bits(&ptr, 8, ip_addr[i]);
    }

    return ptr;
}

// 解析域名（支持压缩指针）
uint8_t *get_domain(uint8_t *buffer, char *name, uint8_t *start)
{
    if (!buffer || !name || !start)
    {
        return buffer;
    }

    uint8_t *ptr = buffer;
    int pos = 0;                    // 结果字符串位置
    int jumped = 0;                 // 是否跳过压缩指针
    uint8_t *original_ptr = buffer; // 原始指针位置，用于压缩指针处理

    // 检查第一个字节是否为压缩指针（初步检查简化处理）
    if (*ptr >= 0xC0)
    {
        // 实际偏移量只有14位，最高位是0xC
        uint16_t offset = ntohs(*(uint16_t *)ptr) & 0x3FFF;
        get_domain(start + offset, name, start);
        return buffer + 2;
    }

    while (*ptr != 0)
    {
        if ((*ptr & 0xC0) == 0xC0)
        {
            // 压缩指针：前两位为11
            if (!jumped)
            {
                original_ptr = ptr + 2;
                jumped = 1;
            }
            uint16_t offset = ntohs(*(uint16_t *)ptr) & 0x3FFF;
            ptr = start + offset; // 跳转后继续读取
        }
        else
        {
            // 普通标签，长度+内容
            uint8_t len = *ptr++; // 长度
            if (pos + len + 1 >= MAX_DOMAIN_NAME_LEN - 1)
            {
                break; // 防止缓冲区溢出
            }

            if (pos > 0)
            {
                name[pos++] = '.';
            }

            // 复制标签内容
            memcpy(name + pos, ptr, len);
            pos += len;
            ptr += len;
        }
    }

    name[pos] = '\0';

    // 如果遇到压缩指针处理后的结束位置
    if (jumped)
    {
        return original_ptr;
    }

    // 如果遇到普通结束标记
    if (*ptr == 0)
    {
        ptr++;
    }

    return ptr;
}

// 设置域名到缓冲区
uint8_t *set_domain(uint8_t *buffer, char *name)
{
    if (!buffer || !name)
    {
        return buffer;
    }

    uint8_t *ptr = buffer;
    char *name_copy = malloc(strlen(name) + 1);
    if (!name_copy)
    {
        return buffer;
    }

    strcpy(name_copy, name);

    char *token = strtok(name_copy, ".");
    while (token != NULL)
    {
        uint8_t len = strlen(token);
        if (len > 63)
        { // DNS标签最大长度为63
            len = 63;
        }
        *ptr++ = len;
        memcpy(ptr, token, len);
        ptr += len;
        token = strtok(NULL, ".");
    }
    *ptr++ = 0; // 结束标记

    free(name_copy);
    return ptr;
}

// 释放DNS消息内存
void free_message(DnsMessage *msg)
{
    if (!msg)
    {
        return;
    }

    // 释放头部
    if (msg->header)
    {
        free(msg->header);
        msg->header = NULL;
    }

    // 释放问题链表
    DnsQuestion *question = msg->questions;
    while (question)
    {
        DnsQuestion *next = question->next;
        if (question->qname)
        {
            free(question->qname);
        }
        free(question);
        question = next;
    }
    msg->questions = NULL;

    // 释放答案链表
    DnsResourceRecord *record = msg->answers;
    while (record)
    {
        DnsResourceRecord *next = record->next;
        if (record->name)
        {
            free(record->name);
        }

        // 根据记录类型释放rdata
        if (record->type == QTYPE_CNAME && record->rdata.cname_record.name)
        {
            free(record->rdata.cname_record.name);
        }
        else if (record->type != QTYPE_A && record->rdata.raw_data)
        {
            free(record->rdata.raw_data);
        }

        free(record);
        record = next;
    }
    msg->answers = NULL;

    // 释放权威记录链表
    record = msg->authorities;
    while (record)
    {
        DnsResourceRecord *next = record->next;
        if (record->name)
        {
            free(record->name);
        }
        if (record->rdata.raw_data)
        {
            free(record->rdata.raw_data);
        }
        free(record);
        record = next;
    }
    msg->authorities = NULL;

    // 释放附加记录链表
    record = msg->additionals;
    while (record)
    {
        DnsResourceRecord *next = record->next;
        if (record->name)
        {
            free(record->name);
        }
        if (record->rdata.raw_data)
        {
            free(record->rdata.raw_data);
        }
        free(record);
        record = next;
    }
    msg->additionals = NULL;
}

// 解析DNS权威记录部分
uint8_t *get_authority(DnsMessage *msg, uint8_t *buffer, uint8_t *start)
{
    if (!msg || !buffer || !start)
    {
        return buffer;
    }

    uint8_t *ptr = buffer;

    for (int i = 0; i < msg->header->nscount; i++)
    {
        char name[MAX_DOMAIN_NAME_LEN] = {0};
        DnsResourceRecord *record = malloc(sizeof(DnsResourceRecord));

        if (!record)
        {
            continue;
        }

        // 解析域名
        ptr = get_domain(ptr, name, start);

        record->name = malloc(strlen(name) + 1);
        if (record->name)
        {
            strcpy(record->name, name);
        }

        // 解析记录头部
        record->type = get_bits(&ptr, 16);
        record->class_code = get_bits(&ptr, 16);
        record->ttl = get_bits(&ptr, 32);
        record->rdlength = get_bits(&ptr, 16);

        // 对于权威记录，通常直接跳过RDATA或存储为原始数据
        record->rdata.raw_data = malloc(record->rdlength);
        if (record->rdata.raw_data)
        {
            memcpy(record->rdata.raw_data, ptr, record->rdlength);
        }
        ptr += record->rdlength;

        // 添加到链表头部
        record->next = msg->authorities;
        msg->authorities = record;
    }

    return ptr;
}

// 解析DNS附加记录部分
uint8_t *get_additional(DnsMessage *msg, uint8_t *buffer, uint8_t *start)
{
    if (!msg || !buffer || !start)
    {
        return buffer;
    }

    uint8_t *ptr = buffer;

    for (int i = 0; i < msg->header->arcount; i++)
    {
        char name[MAX_DOMAIN_NAME_LEN] = {0};
        DnsResourceRecord *record = malloc(sizeof(DnsResourceRecord));

        if (!record)
        {
            continue;
        }

        // 解析域名
        ptr = get_domain(ptr, name, start);

        record->name = malloc(strlen(name) + 1);
        if (record->name)
        {
            strcpy(record->name, name);
        }

        // 解析记录头部
        record->type = get_bits(&ptr, 16);
        record->class_code = get_bits(&ptr, 16);
        record->ttl = get_bits(&ptr, 32);
        record->rdlength = get_bits(&ptr, 16);

        // 对于附加记录，通常直接跳过RDATA或存储为原始数据
        record->rdata.raw_data = malloc(record->rdlength);
        if (record->rdata.raw_data)
        {
            memcpy(record->rdata.raw_data, ptr, record->rdlength);
        }
        ptr += record->rdlength;

        // 添加到链表头部
        record->next = msg->additionals;
        msg->additionals = record;
    }

    return ptr;
}