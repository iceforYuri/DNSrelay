# 大二下计算机网络课程设计
本次 DNS 中继器服务的设计与实现，紧密结合网络协议理论与实际实践，设计一个DNS中继服务器。具体涵盖了报文处理、缓存信息管理、网站拦截、中继、多平台运行等功能。实现过程利用了哈希表、双向链表、LRU机制等实现功能。设计过程深入网络协议相关理论知识，理解DNS相关结构与功能。

## 程序流程
<img width="690" height="618" alt="image" src="https://github.com/user-attachments/assets/d8099895-4171-48d7-bb3f-2dfcf5a5311f" />

## 功能测试
### 初始化
<img width="693" height="207" alt="image" src="https://github.com/user-attachments/assets/d5f460c4-a687-4d30-8c98-9c0150d9fee0" />
运行dnsrelay.exe，启动cmd命令窗口，输入nslookup命令，可以看到此时DNS已设置为127.0.0.1（本地主机），而在-dd调试中可以看到初始化加载文件到缓存中的详细说明。
<img width="693" height="411" alt="image" src="https://github.com/user-attachments/assets/e75b07a3-cc7e-4c0e-be0c-8b7c58daecec" />

### 拦截功能
<img width="693" height="261" alt="image" src="https://github.com/user-attachments/assets/59c789f6-6feb-4aea-a4cd-d7b6edcf8d8f" />
其中查询008.cn以及cctv1.net，可以看到查询结果为查询不到。因为008.cn以及cctv1.net在ip域名对应表中设置为0.0.0.0，如结果所示被拦截。

### 查询和中继
- 在查询本地维护的host表中，获取到的是权威应答
- 当本地查询不到时，会分配ID并发送到上游服务器进行查询，后续在报文返回后也会将IP-域名对应加入缓存表中。

## 多客户端实现
程序无需改动可以在linux下进行编译，使用cmake的结果如下，编译成功。
<img width="693" height="242" alt="image" src="https://github.com/user-attachments/assets/66de8430-d094-4cf6-902d-1740bedcefea" />

## 其他测试
<img width="693" height="359" alt="image" src="https://github.com/user-attachments/assets/21642d58-0ddb-43aa-8967-9aa7f8d462e7" />
其中，对于发送10000条域名的查询测试，完成 9949 条（成功率 99.49%），丢包 51 条（丢包率 0.51%）。平均请求包大小为 28 字节，响应包大小为 79 字节，符合 DNS 协议常规数据量。运行时长 16.204031 秒，查询速率达 613.98 QPS（每秒查询数）。延迟方面，平均 latency 为 0.107860 秒，最小值 0.011586 秒，最大值 1.773274 秒，标准差 0.036707 秒。数据表明，程序在高并发场景下成功率超 99%，查询处理效率稳定，延迟控制在合理区间，整体趋向样例，整体运行稳定性与性能表现符合预期。

# 最终致谢
SD202、Goo-od
