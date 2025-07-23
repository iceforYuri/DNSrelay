#ifdef _WIN32

#include <winsock2.h>
typedef SOCKET my_socket;

void my_setSockAddr(struct sockaddr_in* sockaddr,short family,u_long addr,u_short port);
void my_socketInit();
void my_socketRelease();

#else
#include <sys/socket.h>//定义socket相关函数
#include <arpa/inet.h>//定义IP地址转换相关函数
#include <netinet/in.h>//contains sockaddr_in
#include <arpa/inet.h>//contains inet_ntoa()
#include <netinet/in.h>//contains ntohs()
#define my_socketInit();
#define my_socketRelease();

typedef int my_socket;
void my_setSockAddr(struct sockaddr_in* sockaddr,short family,u_long addr,u_short port);

#endif