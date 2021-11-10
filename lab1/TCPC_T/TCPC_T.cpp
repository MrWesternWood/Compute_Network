#include<WinSock2.h>
#include<iostream>
#include <cstdio>
#include<thread>
#include<string>
#pragma comment(lib,"ws2_32.lib")
using namespace std;

WSADATA wsaData;//存放套接字信息的结构
SOCKET ClientSocket = INVALID_SOCKET;//客户端套接字
SOCKADDR_IN clientaddr = { 0 };//客户端地址
USHORT uPort = 8888;//服务器监听端口
char username[31];
char ip[20];

DWORD WINAPI RevThread(LPVOID lpParameter) {
	char buff[255] = { 0 };
	int res = 0;
	while (1) {
		res = recv(*(SOCKET*)lpParameter, buff, sizeof(buff), 0);
		// 如果没有收到消息，就2s之后再次接收消息
		if (res == SOCKET_ERROR) {
			Sleep(2000);
			continue;
		}
		// 如果接收到非空消息，就输出消息
		if (strlen(buff) != 0) {
			cout << buff << endl << endl;
		}
		else
			// 接收到空消息（服务器在通过发送空消息测试连接状态）
			Sleep(100);
	}
	return 0;
}

DWORD WINAPI SendThread(LPVOID lpParameter) {
	char buff[255] = { 0 };
	int res = 0;
	while (1) {
		cin.getline(buff, 255);
		cout << endl;
		// 如果输入结束信号"quit"，那么将结束信号发送给服务器并关闭套接字，显示退出聊天。
		if (strcmp(buff, "quit") == 0) {
			res = send(*(SOCKET*)lpParameter, buff, sizeof(buff), 0);
			closesocket(ClientSocket);
			cout << "已退出聊天。" << endl << endl;
			WSACleanup();
			return 0;
		}
		// 将输入的消息发送给服务端
		res = send(*(SOCKET*)lpParameter, buff, sizeof(buff), 0);
		if (res == SOCKET_ERROR) {
			cout << "发送消息失败！" << endl << endl;
			return -1;
		}
	}
	return 0;
}

int main() {
	//初始化套接字
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) { //WSAStartup返回0表示设置初始化成功
		cout << "WSAStartup failed!" << endl << endl;
		return -1;
	}
	//创建套接字
	ClientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (ClientSocket == INVALID_SOCKET) {
		cout << "套接字创建失败！" << endl << endl;
		WSACleanup();
		return -1;
	}
	cout << "请输入IP地址：";
	cin.getline(ip, 20);
	cout << endl;
	clientaddr.sin_family = AF_INET;
	clientaddr.sin_port = htons(uPort);//绑定端口号
	clientaddr.sin_addr.S_un.S_addr = inet_addr(ip);
	cout << "请输入用户名：";
	cin.getline(username, 32);
	cout << endl;
	if (connect(ClientSocket, (SOCKADDR*)&clientaddr, sizeof(clientaddr)) == SOCKET_ERROR) {
		cout << "客户端建立连接失败！" << endl << endl;
		closesocket(ClientSocket);
		WSACleanup();
		return -1;
	}
	cout << "客户端建立连接成功！" << endl << endl;
	send(ClientSocket, username, sizeof(username), 0);
	// 创建一个数据接收线程，接受来自服务器的消息转发；
	CreateThread(NULL, 0, &RevThread, &ClientSocket, 0, NULL);
	// 创建一个数据发送线程，可以在聊天室任意时刻发送任意消息；
	CreateThread(NULL, 0, &SendThread, &ClientSocket, 0, NULL);
	// 让主线程休眠，不让它关闭TCP连接
	for (int k = 0;k < 1000;k++) {
		Sleep(1000000000);
	}
	closesocket(ClientSocket);
	WSACleanup();
	return 0;
}
