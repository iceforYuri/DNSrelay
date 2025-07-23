#include "io.h"
// data_struct.h 已经通过 io.h 包含了

// 外部变量声明
int debug_mode = 0;
char IPAddr[MAX_SIZE];
char domain[MAX_SIZE];
char host_file_path[MAX_PATH_LEN] = HOST_PATH; // 默认路径

/**
 * 读取host文件并加载域名-IP映射信息
 */
void read_host()
{
    FILE *host_ptr = fopen(host_file_path, "r");

    if (!host_ptr)
    {
        debug_print1("Error! Can not open hosts file: %s\n", host_file_path);
        exit(1);
    }

    debug_print1("Loading host file: %s\n", host_file_path);
    get_host_info(host_ptr);
    fclose(host_ptr);
}

/**
 * 从文件指针中解析host信息
 * @param ptr 指向host文件的文件指针
 */
void get_host_info(FILE *ptr)
{
    if (!ptr)
    {
        debug_print1("Error: Invalid file pointer\n");
        return;
    }

    int num = 0;
    char line[512];

    // 逐行读取文件
    while (fgets(line, sizeof(line), ptr))
    {
        // 跳过空行和注释行
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r')
        {
            continue;
        } // 解析IP地址和域名
        if (sscanf(line, "%15s %255s", IPAddr, domain) == 2)
        {
            uint8_t this_ip[4];

            // 转换IP地址字符串为字节数组
            transfer_IP(this_ip, IPAddr);

            // 直接添加到缓存作为静态记录
            add_static_record(this_ip, domain);

            num++;

            debug_print2("Loaded: %s -> %s\n", domain, IPAddr);
        }
    }

    debug_print2("%d domain name address info has been loaded.\n\n", num);
}

/**
 * mutex
 * 写入DNS查询日志
 * @param domain 查询的域名
 * @param ip_addr 返回的IP地址，NULL表示未找到或来自远程服务器
 */
void write_log(char *domain, uint8_t *ip_addr)
{
    if (!domain)
    {
        return;
    }

    // 获取当前时间
    time_t currentTime = time(NULL);
    // 将时间转换为本地时间
    struct tm *localTime = localtime(&currentTime);

    // 格式化并打印时间
    char timeString[100];
    strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", localTime);

    my_lockMutex(log_Mutex);

    FILE *fp = fopen(LOG_FILE_PATH, "a");
    if (fp == NULL)
    {
        debug_print1("Log file open failed.\n");
        return;
    }

    debug_print1("Log file open succeed.\n");

    fprintf(fp, "%s  ", timeString);

    fprintf(fp, "%s  ", domain);

    if (ip_addr != NULL)
    {
        fprintf(fp, "%d.%d.%d.%d\n", ip_addr[0], ip_addr[1], ip_addr[2], ip_addr[3]);
    }
    else
    {
        fprintf(fp, "Not found in local. Returned from remote DNS server.\n");
    }

    // 刷新缓冲区并关闭文件
    fflush(fp);
    fclose(fp);

    my_unlockMutex(log_Mutex);
}

/**
 * 将IP字节数组转换为字符串
 * @param ip_array IP字节数组（4字节）
 * @param ip_string 输出的IP字符串缓冲区（至少16字节）
 */
void ip_to_string(uint8_t *ip_array, char *ip_string)
{
    if (!ip_array || !ip_string)
    {
        return;
    }

    snprintf(ip_string, 16, "%d.%d.%d.%d",
             ip_array[0], ip_array[1], ip_array[2], ip_array[3]);
}