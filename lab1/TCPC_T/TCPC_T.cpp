#include<WinSock2.h>
#include<iostream>
#include <cstdio>
#include<thread>
#include<string>
#pragma comment(lib,"ws2_32.lib")
using namespace std;

WSADATA wsaData;//����׽�����Ϣ�Ľṹ
SOCKET ClientSocket = INVALID_SOCKET;//�ͻ����׽���
SOCKADDR_IN clientaddr = { 0 };//�ͻ��˵�ַ
USHORT uPort = 8888;//�����������˿�
char username[31];
char ip[20];

DWORD WINAPI RevThread(LPVOID lpParameter) {
	char buff[255] = { 0 };
	int res = 0;
	while (1) {
		res = recv(*(SOCKET*)lpParameter, buff, sizeof(buff), 0);
		// ���û���յ���Ϣ����2s֮���ٴν�����Ϣ
		if (res == SOCKET_ERROR) {
			Sleep(2000);
			continue;
		}
		// ������յ��ǿ���Ϣ���������Ϣ
		if (strlen(buff) != 0) {
			cout << buff << endl << endl;
		}
		else
			// ���յ�����Ϣ����������ͨ�����Ϳ���Ϣ��������״̬��
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
		// �����������ź�"quit"����ô�������źŷ��͸����������ر��׽��֣���ʾ�˳����졣
		if (strcmp(buff, "quit") == 0) {
			res = send(*(SOCKET*)lpParameter, buff, sizeof(buff), 0);
			closesocket(ClientSocket);
			cout << "���˳����졣" << endl << endl;
			WSACleanup();
			return 0;
		}
		// ���������Ϣ���͸������
		res = send(*(SOCKET*)lpParameter, buff, sizeof(buff), 0);
		if (res == SOCKET_ERROR) {
			cout << "������Ϣʧ�ܣ�" << endl << endl;
			return -1;
		}
	}
	return 0;
}

int main() {
	//��ʼ���׽���
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) { //WSAStartup����0��ʾ���ó�ʼ���ɹ�
		cout << "WSAStartup failed!" << endl << endl;
		return -1;
	}
	//�����׽���
	ClientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (ClientSocket == INVALID_SOCKET) {
		cout << "�׽��ִ���ʧ�ܣ�" << endl << endl;
		WSACleanup();
		return -1;
	}
	cout << "������IP��ַ��";
	cin.getline(ip, 20);
	cout << endl;
	clientaddr.sin_family = AF_INET;
	clientaddr.sin_port = htons(uPort);//�󶨶˿ں�
	clientaddr.sin_addr.S_un.S_addr = inet_addr(ip);
	cout << "�������û�����";
	cin.getline(username, 32);
	cout << endl;
	if (connect(ClientSocket, (SOCKADDR*)&clientaddr, sizeof(clientaddr)) == SOCKET_ERROR) {
		cout << "�ͻ��˽�������ʧ�ܣ�" << endl << endl;
		closesocket(ClientSocket);
		WSACleanup();
		return -1;
	}
	cout << "�ͻ��˽������ӳɹ���" << endl << endl;
	send(ClientSocket, username, sizeof(username), 0);
	// ����һ�����ݽ����̣߳��������Է���������Ϣת����
	CreateThread(NULL, 0, &RevThread, &ClientSocket, 0, NULL);
	// ����һ�����ݷ����̣߳�����������������ʱ�̷���������Ϣ��
	CreateThread(NULL, 0, &SendThread, &ClientSocket, 0, NULL);
	// �����߳����ߣ��������ر�TCP����
	for (int k = 0;k < 1000;k++) {
		Sleep(1000000000);
	}
	closesocket(ClientSocket);
	WSACleanup();
	return 0;
}
