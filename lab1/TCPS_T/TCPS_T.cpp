#include<WinSock2.h>
#include<iostream>
#include<cstdio>
#include<string>
#include<thread>
#include"ClientLinkList.h"
#pragma comment(lib,"ws2_32.lib")

using namespace std;
SOCKET servers = INVALID_SOCKET;//������׽���
SOCKADDR_IN clientaddr = { 0 };//�ͻ��˵�ַ
SOCKADDR_IN serveraddr = { 0 };	//����˵�ַ
WSADATA wsaData = { 0 };
USHORT uPort = 8888;
int Maxclient = 10;
int clientaddrlen = sizeof(clientaddr);

//��������
DWORD WINAPI SendThread(LPVOID lpParameter) {
	pclient p = (pclient)lpParameter;
	// �ڷ������������Ϣ
	cout << p->username << "��" << p->buff << endl << endl;
	// ��������ÿ���ͻ���ת����Ϣ
	senddata(p);
	return 0;
}

//��������
DWORD WINAPI RevThread(LPVOID lpParameter) {
	int res = 0;
	while (1) {
		pclient p = (pclient)lpParameter;
		res = recv(p->clients, p->buff, sizeof(p->buff), 0);
		if (res == SOCKET_ERROR) {
			return -1;
		}
		// ����ǿͻ��˷������˳��ź�"quit"�����˳��߳�
		// ��Ϊ�ͻ����Լ��˳��ر��׽��֣����Է������ļ������״̬���̻߳��⵽���㲥�����пͻ���
		if (strcmp(p->buff, "quit") == 0) {
			return 0;
		}
		else {
			// ����һ������ת���̣߳��Կͻ����е�ÿ���ͻ��˽��յ������ݽ���ת����
			CreateThread(NULL, 0, &SendThread, p, 0, NULL);
		}
	}
	return 0;
}

//��������
DWORD WINAPI ManagerThread(LPVOID lpParameter) {
	// ÿ��2s���һ�������пͻ��˵�����״̬���Ա㷢�������жϵĿͻ���
	while (1) {
		checkconnection();
		Sleep(2000);
	}
	return 0;
}
//��������
DWORD WINAPI AcceptThread(LPVOID lpParameter) {
	// ʹ���̹߳�������״̬��ѭ����������еĿͻ����Ƿ������ӶϿ��������
	CreateThread(NULL, 0, &ManagerThread, NULL, 0, 0);
	init();
	while (1) {
		pclient Client = (pclient)malloc(sizeof(client));
		Client->clients = accept(servers, (SOCKADDR*)&clientaddr, &clientaddrlen);
		if (Client->clients == INVALID_SOCKET) {
			cout << "��������" << endl << endl;
			closesocket(servers);
			WSACleanup();
			return -1;
		}
		// �����û����û���
		recv(Client->clients, Client->username, sizeof(Client->username), 0);
		// ���ÿͻ���IP��ַ����ʶ�ͻ��˵�Ψһ��ʶ��flag���Լ��˿ں�
		memcpy(Client->ip, inet_ntoa(clientaddr.sin_addr), sizeof(Client->ip));
		Client->flag = Client->clients;
		Client->port = htons(clientaddr.sin_port);
		// ���ͻ��˼�������
		addclient(Client);
		cout << "�ͻ���" << Client->username << "���ӷ������ɹ���" << endl << endl;
		pclient p = getheadnode();
		// ������еĿͻ����׽��ֽ���������ݽ����߳�
		while (p = p->next) {
			CreateThread(NULL, 0, &RevThread, p, 0, NULL);
		}
	}
	return 0;
}

int main() {
	// ��ʼ���׽���
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) { //WSAStartup����0��ʾ���ó�ʼ���ɹ�
		cout << "WSAStartup failed!" << endl << endl;
		return -1;
	}
	// �����׽���
	servers = socket(AF_INET, SOCK_STREAM, 0);
	if (servers == INVALID_SOCKET) {
		cout << "�׽��ִ���ʧ�ܣ�" << endl << endl;
		WSACleanup();
		return -1;
	}
	// ���÷�������ַ
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(uPort);//�󶨶˿ں�
	serveraddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);//�κοͻ��˶����������������
	// �󶨷�����
	if (bind(servers, (SOCKADDR*)&serveraddr, sizeof(serveraddr)) == SOCKET_ERROR) {
		cout << "��ʧ�ܣ�" << endl << endl;
		closesocket(servers);
		WSACleanup();
		return -1;
	}
	
	//�������޿ͻ�������
	if (listen(servers, Maxclient) == SOCKET_ERROR) {
		cout << "��������" << endl << endl;
		closesocket(servers);
		WSACleanup();
		return -1;
	}
	cout << "�ȴ����ӡ���" << endl << endl;
	// ����һ���������������̣߳����Ͻ������Կͻ��˵���������
	CreateThread(NULL, 0, &AcceptThread, NULL, 0, 0);
	// �����߳����ߣ��������ر�TCP����
	for (int k = 0;k < 1000;k++) {
		Sleep(1000000000);
	}
	cleanclient();
	closesocket(servers);
	WSACleanup();
	return 0;
}