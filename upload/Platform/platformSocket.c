#include "platformSocket.h"
#ifdef _WIN32

void my_setSockAddr(struct sockaddr_in* sockaddr, short family, u_long addr, u_short port) {

    sockaddr->sin_family = family;
    sockaddr->sin_addr.S_un.S_addr = addr;
    sockaddr->sin_port = htons(port);
}

// 进行socket初始化，并使用2.2版本的socket
void my_socketInit()
{
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2,2), &wsaData);
}
void my_socketRelease()
{
    WSACleanup();
}

#else

void my_setSockAddr(struct sockaddr_in* sockaddr, short family, u_long addr, u_short port) {
    sockaddr->sin_family = family;
    sockaddr->sin_addr.s_addr = addr;
    sockaddr->sin_port = htons(port);
}
#endif