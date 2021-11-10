#include "ClientLinkList.h"
#include<iostream>
using namespace std;

pclient head = (pclient)malloc(sizeof(client));
void init() {
	head->next = NULL;
}

pclient getheadnode() {
	return head;
}

void addclient(pclient c) {
	// ���ͻ��˼�������
	c->next = head->next;
	head->next = c;
	pclient p = getheadnode();
	strcpy(c->buff, c->username);
	strcat(c->buff, "��������");
	// �����������еĿͻ��˹㲥�ÿͻ��˼�������
	while (p = p->next) {
		if (p->flag != c->flag) {
			send(p->clients, c->buff, sizeof(c->buff), 0);
		}
	}
}

bool deleteclient(UINT_PTR flag) {
	pclient pcur = head->next;
	pclient pre = head;
	while (pcur) {
		if (pcur->flag == flag) {
			pre->next = pcur->next;
			closesocket(pcur->clients);
			delete pcur;
			return true;
		}
		pre = pcur;
		pcur = pcur->next;
	}
	return false;
}

void cleanclient() {
	pclient pcur = head->next;
	pclient pre = head;
	while (pcur) {
		pclient p = pcur;
		pre->next = p->next;
		delete p;
		pcur = pre->next;
	}
}

void senddata(pclient c) {
	pclient p = getheadnode();
	while (p = p->next) {
		if (p->flag != c->flag) {
			// ����Ϣת���� username��text����ʽ
			strcpy(p->buff, c->username);
			strcat(p->buff, "��");
			strcat(p->buff, c->buff);
			send(p->clients, p->buff, sizeof(p->buff), 0);
		}
	}
}

void checkconnection() {
	pclient pc = getheadnode();
	while (pc = pc->next) {
		// ͨ��send��������һ������Ϣ���ж��Ƿ�Ͽ�����
		if (send(pc->clients, "", sizeof(""), 0) == SOCKET_ERROR) {
			if (pc->clients != 0) {
				cout << pc->username << "ʧȥ���ӡ���" << endl << endl;
				strcpy(pc->buff, pc->username);
				strcat(pc->buff, "�˳�����");
				pclient p = getheadnode();
				// �����пͻ��˹㲥�ÿͻ����˳��������Ϣ
				while (p = p->next) {
					if (p->flag != pc->flag) {
						send(p->clients, pc->buff, sizeof(pc->buff), 0);
					}
				}
				// �ر��׽���
				closesocket(pc->clients);
				// ��������ɾ������ͻ���
				deleteclient(pc->flag);
				break;
			}
		}
	}
}

