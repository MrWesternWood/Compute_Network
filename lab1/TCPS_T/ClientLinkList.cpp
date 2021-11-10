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
	// 将客户端加入链表
	c->next = head->next;
	head->next = c;
	pclient p = getheadnode();
	strcpy(c->buff, c->username);
	strcat(c->buff, "加入聊天");
	// 向链表中所有的客户端广播该客户端加入聊天
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
			// 将消息转发成 username：text的形式
			strcpy(p->buff, c->username);
			strcat(p->buff, "：");
			strcat(p->buff, c->buff);
			send(p->clients, p->buff, sizeof(p->buff), 0);
		}
	}
}

void checkconnection() {
	pclient pc = getheadnode();
	while (pc = pc->next) {
		// 通过send函数发送一个空消息来判断是否断开连接
		if (send(pc->clients, "", sizeof(""), 0) == SOCKET_ERROR) {
			if (pc->clients != 0) {
				cout << pc->username << "失去连接……" << endl << endl;
				strcpy(pc->buff, pc->username);
				strcat(pc->buff, "退出聊天");
				pclient p = getheadnode();
				// 向所有客户端广播该客户端退出聊天的消息
				while (p = p->next) {
					if (p->flag != pc->flag) {
						send(p->clients, pc->buff, sizeof(pc->buff), 0);
					}
				}
				// 关闭套接字
				closesocket(pc->clients);
				// 在链表中删除这个客户端
				deleteclient(pc->flag);
				break;
			}
		}
	}
}

