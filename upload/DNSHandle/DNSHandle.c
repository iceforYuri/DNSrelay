#include "DNSHandle.h"

static bool QueryForLocal(char *name, uint8_t ip_addrs[][4], int *ip_count, int *is_authoritative)
{

    // 使用新的多IP缓存查询
    *ip_count = query_cache(name, ip_addrs, 10, is_authoritative);

    if (*ip_count > 0)
    {
        if (debug_mode == 2)
        {
            debug_print2("Local query result: %s -> %d IP(s)\n", name, *ip_count);
            for (int i = 0; i < *ip_count; i++)
            {
                debug_print2("  IP %d: %d.%d.%d.%d\n", i + 1,
                             ip_addrs[i][0], ip_addrs[i][1], ip_addrs[i][2], ip_addrs[i][3]);
            }
        }
        return true;
    }

    return false;
}

static bool SendToOtherServer(Task *t)
{

    uint16_t oldId = ntohs(*((uint16_t *)(t->buf)));
    debug_print2("old ID:%u\n", oldId);

    uint16_t server_ID = set_ID(oldId, t->clientAddr);
    if (server_ID == 0) // set_ID失败返回0，不是ID_LIST_SIZE
    {

        debug_print1("No available ID for upstream server, dropping query.\n");

        return false;
    }

    debug_print2("new ID:%u\n", server_ID);

    *(uint16_t *)(t->buf) = htons(server_ID);

    sendto(servSock, t->buf, t->len, 0, (struct sockaddr *)&remoteSockAddr, sizeof(remoteSockAddr));
    return true;
}

static void SendResponse(DnsMessage *dnsM, uint8_t ip_addrs[][4], int ip_count, struct sockaddr_in *client_addr, int is_authoritative)
{
    uint8_t buffer_response[MAX_DNS_SIZE];

    // 修改DNS头部以反映多个答案记录
    dnsM->header->ancount = ip_count;

    uint8_t *response_ptr = set_message(dnsM, buffer_response, ip_addrs, ip_count, is_authoritative);
    int len = response_ptr - buffer_response;

    sendto(servSock, buffer_response, len, 0,
           (struct sockaddr *)client_addr, sizeof(*client_addr));

    debug_print2("send to client with %d IP(s)\n", ip_count);
}

void DNSHandle(Task *t)
{
    DnsMessage dnsM;
    uint8_t *ptr = (uint8_t *)(t->buf); // 处理报文的函数需要使用，指向当前报文正在处理的位置
    uint8_t ip_addr[4];

    switch (t->buf[3] & 0x80)
    {
    case QUERY_MESSAGE:
        // printf("go to 1\n");
        get_message(&dnsM, ptr, (uint8_t *)(t->buf));
        if (dnsM.header->opcode == STANDARD_QUERY)
        {
            // printf("go to 1.1\n");
            // TODO：如果不止有一个问题的话，需要加上for循环，并且需要更改后续处理流程，相当复杂
            if (dnsM.questions->qtype == RR_A)
            {
                // 查询缓存，获取所有IP地址
                uint8_t ip_addrs[10][4]; // 最多支持10个IP地址
                int ip_count;
                int is_authoritative;

                if (!QueryForLocal(dnsM.questions->qname, ip_addrs, &ip_count, &is_authoritative))
                {
                    debug_print1("%d: @send to upstream %s, TYPE: %d, CLASS: %d\n",
                                 message_count++, dnsM.questions->qname,
                                 dnsM.questions->qtype, dnsM.questions->qclass);
                    if (!SendToOtherServer(t))
                    {
                        free_message(&dnsM);
                    }
                    return;
                }
                else
                {
                    SendResponse(&dnsM, ip_addrs, ip_count, &(t->clientAddr), is_authoritative);
                    debug_print1("%d: *find from cache  %s, TYPE: %d, CLASS: %d\n",
                                 message_count++, dnsM.questions->qname,
                                 dnsM.questions->qtype, dnsM.questions->qclass);
                    free_message(&dnsM);

                    return;
                }
            }
            else
            {
                SendToOtherServer(t);
                debug_print1("%d: @send to upstream %s, TYPE: %d, CLASS: %d\n",
                             message_count++, dnsM.questions->qname,
                             dnsM.questions->qtype, dnsM.questions->qclass);

                free_message(&dnsM);
            }
        }
        else
        {
            SendToOtherServer(t);
        }
        free_message(&dnsM);
        break;
    case RESPONSE_MESSAGE:
        // printf("go to 2\n");
        get_message(&dnsM, ptr, (uint8_t *)(t->buf));
        debug_print2("received from upstream server\n");

        uint16_t server_ID = dnsM.header->id;
        uint16_t client_ID = 0;
        struct sockaddr_in original_client_addr;

        if (get_client_info(server_ID, &original_client_addr, &client_ID) == 0)
        {
            debug_print1("\nno match id\n");
            free_message(&dnsM);
            return;
        }

        // 将响应报文发送给原始客户端
        sendto(servSock, t->buf, t->len, 0,
               (struct sockaddr *)&original_client_addr, sizeof(original_client_addr));

        // 直接修改原始buffer的ID字段，恢复客户端ID
        *(uint16_t *)(t->buf) = htons(client_ID); // 网络字节序        // 将有效的DNS响应添加到缓存
        if (dnsM.header->rcode == RCODE_NO_ERROR && dnsM.header->ancount > 0 && dnsM.answers)
        {
            // 收集所有A记录的IP地址
            uint8_t ip_addresses[10][4]; // 最多支持10个IP地址
            uint32_t ttl[10];            // 存储每个IP的TTL
            int ip_count = 0;

            // 获取上游响应的权威性标识
            int upstream_authoritative = dnsM.header->aa;

            DnsResourceRecord *answer = dnsM.answers;

            // 遍历所有答案记录
            while (answer && ip_count < 10)
            {
                if (answer->type == QTYPE_A && answer->rdlength == 4)
                {
                    memcpy(ip_addresses[ip_count], answer->rdata.a_record.ip_addr, 4);
                    ttl[ip_count] = answer->ttl; // 保存TTL
                    ip_count++;

                    debug_print2("Found A record: %s -> %d.%d.%d.%d\n",
                                 dnsM.questions->qname,
                                 answer->rdata.a_record.ip_addr[0], answer->rdata.a_record.ip_addr[1],
                                 answer->rdata.a_record.ip_addr[2], answer->rdata.a_record.ip_addr[3]);
                }

                answer = answer->next;
            }

            // 如果找到了A记录，使用多IP更新缓存，并传递权威性信息
            if (ip_count > 0)
            {
                update_cache(ip_addresses, ip_count, ttl, dnsM.questions->qname, upstream_authoritative);

                debug_print2("Added %d IP(s) to cache for domain: %s (authoritative: %s)\n",
                             ip_count, dnsM.questions->qname, upstream_authoritative ? "yes" : "no");
            }
        }

        // // 将响应报文发送给原始客户端
        // sendto(servSock, t->buf, t->len, 0,
        //        (struct sockaddr *)&original_client_addr, sizeof(original_client_addr));

        // 写入日志
        // write_log(dnsM.questions->qname, NULL);

        debug_print2("Response forwarded (server_ID=%d -> client_ID=%d) to client: %s:%d\n",
                     server_ID, client_ID,
                     inet_ntoa(original_client_addr.sin_addr),
                     ntohs(original_client_addr.sin_port));

        // 释放服务器ID
        if (delete_ID(server_ID) == 1)
        {

            debug_print2("Released server ID %d\n", server_ID);
        }

        free_message(&dnsM);
        break;
    }
}
