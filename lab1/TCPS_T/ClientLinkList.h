#include<WinSock2.h>
// 客户端结构
typedef struct client {
	SOCKET clients; // 客户端套接字
	char buff[255]; // 数据缓冲区
	char username[31]; // 客户端用户名
	char ip[20]; // 客户端IP
	unsigned short port; // 客户端端口
	UINT_PTR flag; // 标记客户端，用来区分不同客户端
	client* next;
}*pclient;

//初始化链表
void init();
//获取头结点
pclient getheadnode();
//添加一个客户端
void addclient(pclient c);
//删除一个客户端
bool deleteclient(UINT_PTR flag);
//给链表里除了客户端c的所有的客户端发消息
void senddata(pclient c);
//检查连接状态
void checkconnection();
//清空链表
void cleanclient();