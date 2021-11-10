#include<WinSock2.h>
#include<iostream>
#include<cstdio>
#include<string>
#include<thread>
#include"ClientLinkList.h"
#pragma comment(lib,"ws2_32.lib")

using namespace std;
SOCKET servers = INVALID_SOCKET;//服务端套接字
SOCKADDR_IN clientaddr = { 0 };//客户端地址
SOCKADDR_IN serveraddr = { 0 };	//服务端地址
WSADATA wsaData = { 0 };
USHORT uPort = 8888;
int Maxclient = 10;
int clientaddrlen = sizeof(clientaddr);

//发送数据
DWORD WINAPI SendThread(LPVOID lpParameter) {
	pclient p = (pclient)lpParameter;
	// 在服务器中输出消息
	cout << p->username << "：" << p->buff << endl << endl;
	// 向链表中每个客户端转发消息
	senddata(p);
	return 0;
}

//接受数据
DWORD WINAPI RevThread(LPVOID lpParameter) {
	int res = 0;
	while (1) {
		pclient p = (pclient)lpParameter;
		res = recv(p->clients, p->buff, sizeof(p->buff), 0);
		if (res == SOCKET_ERROR) {
			return -1;
		}
		// 如果是客户端发来的退出信号"quit"，就退出线程
		// 因为客户端以及退出关闭套接字，所以服务器的检测连接状态的线程会检测到并广播给所有客户端
		if (strcmp(p->buff, "quit") == 0) {
			return 0;
		}
		else {
			// 开启一个数据转发线程，对客户端中的每个客户端将收到的数据进行转发。
			CreateThread(NULL, 0, &SendThread, p, 0, NULL);
		}
	}
	return 0;
}

//管理连接
DWORD WINAPI ManagerThread(LPVOID lpParameter) {
	// 每隔2s检查一遍链表中客户端的连接状态，以便发现连接中断的客户端
	while (1) {
		checkconnection();
		Sleep(2000);
	}
	return 0;
}
//接受请求
DWORD WINAPI AcceptThread(LPVOID lpParameter) {
	// 使用线程管理连接状态，循环检测链表中的客户端是否发生连接断开的情况；
	CreateThread(NULL, 0, &ManagerThread, NULL, 0, 0);
	init();
	while (1) {
		pclient Client = (pclient)malloc(sizeof(client));
		Client->clients = accept(servers, (SOCKADDR*)&clientaddr, &clientaddrlen);
		if (Client->clients == INVALID_SOCKET) {
			cout << "监听出错！" << endl << endl;
			closesocket(servers);
			WSACleanup();
			return -1;
		}
		// 接受用户端用户名
		recv(Client->clients, Client->username, sizeof(Client->username), 0);
		// 设置客户端IP地址，表识客户端的唯一标识符flag，以及端口号
		memcpy(Client->ip, inet_ntoa(clientaddr.sin_addr), sizeof(Client->ip));
		Client->flag = Client->clients;
		Client->port = htons(clientaddr.sin_port);
		// 将客户端加入链表
		addclient(Client);
		cout << "客户端" << Client->username << "连接服务器成功！" << endl << endl;
		pclient p = getheadnode();
		// 针对所有的客户端套接字建立多个数据接收线程
		while (p = p->next) {
			CreateThread(NULL, 0, &RevThread, p, 0, NULL);
		}
	}
	return 0;
}

int main() {
	// 初始化套接字
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) { //WSAStartup返回0表示设置初始化成功
		cout << "WSAStartup failed!" << endl << endl;
		return -1;
	}
	// 创建套接字
	servers = socket(AF_INET, SOCK_STREAM, 0);
	if (servers == INVALID_SOCKET) {
		cout << "套接字创建失败！" << endl << endl;
		WSACleanup();
		return -1;
	}
	// 设置服务器地址
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(uPort);//绑定端口号
	serveraddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);//任何客户端都能连接这个服务器
	// 绑定服务器
	if (bind(servers, (SOCKADDR*)&serveraddr, sizeof(serveraddr)) == SOCKET_ERROR) {
		cout << "绑定失败！" << endl << endl;
		closesocket(servers);
		WSACleanup();
		return -1;
	}
	
	//监听有无客户端连接
	if (listen(servers, Maxclient) == SOCKET_ERROR) {
		cout << "监听出错！" << endl << endl;
		closesocket(servers);
		WSACleanup();
		return -1;
	}
	cout << "等待连接……" << endl << endl;
	// 开启一个接受连接请求线程，不断接受来自客户端的连接请求
	CreateThread(NULL, 0, &AcceptThread, NULL, 0, 0);
	// 让主线程休眠，不让它关闭TCP连接
	for (int k = 0;k < 1000;k++) {
		Sleep(1000000000);
	}
	cleanclient();
	closesocket(servers);
	WSACleanup();
	return 0;
}