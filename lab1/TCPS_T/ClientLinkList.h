#include<WinSock2.h>
// �ͻ��˽ṹ
typedef struct client {
	SOCKET clients; // �ͻ����׽���
	char buff[255]; // ���ݻ�����
	char username[31]; // �ͻ����û���
	char ip[20]; // �ͻ���IP
	unsigned short port; // �ͻ��˶˿�
	UINT_PTR flag; // ��ǿͻ��ˣ��������ֲ�ͬ�ͻ���
	client* next;
}*pclient;

//��ʼ������
void init();
//��ȡͷ���
pclient getheadnode();
//���һ���ͻ���
void addclient(pclient c);
//ɾ��һ���ͻ���
bool deleteclient(UINT_PTR flag);
//����������˿ͻ���c�����еĿͻ��˷���Ϣ
void senddata(pclient c);
//�������״̬
void checkconnection();
//�������
void cleanclient();