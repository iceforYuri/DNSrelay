#include "io.h"
#include "header.h"
#include "platformSocket.h"
#include "multiThread.h"
#include "DNSHandle.h"

my_socket servSock;
struct sockaddr_in servSockAddr, remoteSockAddr;
extern int debug_mode;
int message_count;

int main(int argc, char *argv[])
{
    // 1. 命令行参数初始化
    int argi = 1;
    char remoteIP[16];
    strcpy(remoteIP, "10.3.9.6");
    if (argc > 1)
    {
        for (int i = 1; i < argc; i++)
        {
            if (strcmp(argv[i], "-dd") == 0)
            {
                debug_mode = 2;
                // 优先级最高，遇到-dd直接break
                argi = i + 1;
                break;
            }
            else if (strcmp(argv[i], "-d") == 0 && debug_mode < 2)
            {
                debug_mode = 1;
                argi = i + 1;
            }
        }
    }
    if (argi < argc)
    { // 检查是否有 IP 地址参数（用 inet_addr 判断是否为有效IP）
        unsigned long ip = inet_addr(argv[argi]);
        if (ip != INADDR_NONE)
        {
            strcpy(remoteIP, argv[argi]);
            argi++;
        }
    }
    if (argi < argc)
    {
        // 剩下的参数当作文件路径
        strncpy(host_file_path, argv[argi], MAX_PATH_LEN - 1);
        host_file_path[MAX_PATH_LEN - 1] = '\0';
    }

    message_count = 0;
    printf("Debug mode: ");
    switch (debug_mode)
    {
    case 0:
        printf("none debug output.\n");
        break;
    case 1:
        printf("normal debug output.\n");
        break;
    case 2:
        printf("most debug output.\n");
        break;
    }

    // 2. Socket初始化

    my_socketInit();

    servSock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

    my_setSockAddr(&servSockAddr, AF_INET, INADDR_ANY, DNS_PORT);
    my_setSockAddr(&remoteSockAddr, AF_INET, inet_addr(remoteIP), DNS_PORT);
    printf("Now you are using upstream name sever: %s\n", remoteIP);

    // inet将点分十进制的IPv4字符串转换为网络字节序的32位无符号整数（in_addr_t）

    printf("Bind UDP port 53 ...");
    if (bind(servSock, (struct sockaddr *)&servSockAddr, sizeof(servSockAddr)) == -1)
    {
        printf("failed\n");
        exit(0);
    }
    else
    {
        printf("OK!\n");
    }

    // 3. 线程初始化

    // 初始化锁和信号量
    initLockAndSemaphore();

    // 初始化线程池
    my_thread *threads[THREAD_POOL_SIZE];
    for (int i = 0; i < THREAD_POOL_SIZE; i++)
    {
        threads[i] = my_createThread(workerThread, DNSHandle);
    }

    // 4. 缓存动态表,本地静态表,ID表初始化
    init_ID_list();
    init_cache();
    read_host();

    printf("Initalization completed, starting operation.\n\n");

    // 5. 主线程负责接收数据并投递到任务队列
    for (;;)
    {
        Task t;
        t.clientAddrLen = sizeof(t.clientAddr);

        t.len = recvfrom(servSock, t.buf, SIZE, 0,
                         (struct sockaddr *)&t.clientAddr, &(t.clientAddrLen));

        if (t.len < 0)
        {
            perror("recvfrom");
            continue;
        }
        addTask(&t);
    }

    my_socketRelease();
    return 0;
}