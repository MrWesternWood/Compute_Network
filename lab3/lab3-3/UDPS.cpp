#include <iostream>
#include <fstream>
#include<WinSock2.h>
#include<string>
#pragma comment(lib,"ws2_32.lib")
using namespace std;

WSADATA wsaData = { 0 };
string send_ip; //发送端IP
u_short send_port; //发送端端口
string receive_ip;
u_short receive_port;
SOCKET receiveSocket = INVALID_SOCKET;//客户端套接字
SOCKADDR_IN receiveaddr = { 0 }; //发送端地址
SOCKADDR_IN sendaddr = { 0 }; //发送端地址
int len = sizeof(sendaddr);
int seq;
int ack;
char filename;
int wave = 0;
const int packetsize = 14000;

void setaddr(SOCKADDR_IN* addr, string ip, int port) {
    addr->sin_family = AF_INET;
    addr->sin_port = htons(port);
    addr->sin_addr.S_un.S_addr = inet_addr(ip.c_str());
}

u_short checksum(u_short* buf, int count) {
    register u_long sum = 0;
    while (count--) {
        sum += *buf++;
        if (sum & 0xFFFF0000) {
            sum &= 0xFFFF;
            sum++;
        }
    }
    return ~(sum & 0xFFFF);
}

void setPacketHead(char* sendBuf) {
    sendBuf[0] = 0;//seq
    sendBuf[1] = 0;//ack
    sendBuf[2] = 0;//ACK
    sendBuf[3] = 0;//SYN
    sendBuf[4] = 0;//长度
    sendBuf[5] = 0;//长度
    sendBuf[6] = 0;//校验和
    sendBuf[7] = 0;//校验和
}

int handshake() {
    char sendBuf[8] = { 0 };
    char receiveBuf[8] = { 0 };
    int res = 0;
    while (true) {
        res = recvfrom(receiveSocket, receiveBuf, 8, 0, (SOCKADDR*)&sendaddr, &len);
        if (res == SOCKET_ERROR) {
            Sleep(2000);
            continue;
        }
        if (receiveBuf[3] == 1) {//即数据报的SYN标志位为1，标志握手报文
            //设置数据包发送报头
            setPacketHead(sendBuf);
            u_short buf[5] = { 0 };
            int i;
            for (i = 0;i < 8;i += 2) {
                buf[i / 2] = (u_char)receiveBuf[i] << 8;
                if ((i + 1) < 8) {
                    buf[i / 2] += (u_char)receiveBuf[i + 1];
                }
            }
            u_short checks = checksum(buf, i / 2);
            if (checks == 0) {//校验和为0，数据包传输正确
                //获取数据报中的seq
                seq = (u_char)receiveBuf[0];
                sendBuf[0] = seq;
                //确认序号ack=seq+1
                ack = seq + 1;
                sendBuf[1] = ack;
                //标志位SYN和ACK置1
                sendBuf[2] = 1;//ACK
                sendBuf[3] = 1;//SYN
                sendBuf[6] = checks >> 8;//校验和
                sendBuf[7] = checks & 0xFF;//校验和
                //发送握手确认报文
                sendto(receiveSocket, sendBuf, 8, 0, (SOCKADDR*)&sendaddr, len);
                cout << "shake hand successfully" << endl;
                return 1;
            }
            else {
                cout << "shake hand failed" << endl;
                break;
            }
        }
        else {
            cout << "shake hand failed" << endl;
            break;
        }
    }
    return 0;
}

void recvdata() {
    cout << "start to receive data..." << endl;
    seq = 0;
    ack = 1;
    int k = 1;
    ofstream out;
    char sendBuf[8] = { 0 };
    char* receiveBuf = new char[packetsize];
    memset(receiveBuf, 0, packetsize);
    while (true) {
        filename = '0' + k;
        int res = 0;
        while (true) {
            res = recvfrom(receiveSocket, receiveBuf, packetsize, 0, (SOCKADDR*)&sendaddr, &len);
            if (res == SOCKET_ERROR) {
                Sleep(2000);
                continue;
            }
            //挥手报文
            if (receiveBuf[3] == 2) {//挥手
                setPacketHead(sendBuf);
                u_short buf[5] = { 0 };
                int i;
                for (i = 0;i < 8;i += 2) {
                    buf[i / 2] = (u_char)receiveBuf[i] << 8;
                    if ((i + 1) < 8) {
                        buf[i / 2] += (u_char)receiveBuf[i + 1];
                    }
                }
                u_short checks = checksum(buf, i / 2);
                if (checks == 0) {
                    seq = (u_char)receiveBuf[0];
                    sendBuf[0] = seq;
                    ack = seq + 1;
                    sendBuf[1] = ack;
                    sendBuf[2] = 1;//ACK
                    sendBuf[3] = 2;//SYN
                    sendBuf[6] = checks >> 8;//校验和
                    sendBuf[7] = checks & 0xFF;//校验和
                    sendto(receiveSocket, sendBuf, 8, 0, (SOCKADDR*)&sendaddr, len);
                    cout << "wave hand successfully" << endl;
                    return;
                }
                else {
                    cout << "wave hand failed" << endl;
                    return;
                }
            }
            //cout << "receive " << (int)((u_char)receiveBuf[0]) << endl;
            if (receiveBuf[3] != 2 && receiveBuf[3] != 1 && seq % 256 == (u_char)receiveBuf[0]) {
                setPacketHead(sendBuf);
                u_short* buf = new u_short[packetsize / 2 + 1];
                memset(buf, 0, packetsize / 2 + 1);
                int length = ((u_char)receiveBuf[4] << 8) + (u_char)receiveBuf[5];
                int i;
                for (i = 0;i < 8 + length;i += 2) {
                    buf[i / 2] = ((u_char)receiveBuf[i] << 8);
                    if ((i + 1) < 8 + length) {
                        buf[i / 2] += (u_char)receiveBuf[i + 1];
                    }
                }
                u_short checks = checksum(buf, i / 2);
                if (checks == 0) {
                    if (seq == 0) {
                        out.open(&filename, ifstream::out | ios::binary | ios::app);
                    }
                    seq = (u_char)receiveBuf[0] + 1;//期望序号
                    ack = (u_char)receiveBuf[1];
                    char* buffer = new char[length];
                    for (int j = 0;j < length;j++) {
                        buffer[j] = receiveBuf[8 + j];
                    }
                    out.write(buffer, length);
                    sendBuf[0] = (u_char)seq % 256;
                    sendBuf[1] = (u_char)ack;
                    sendBuf[2] = 1;//ACK
                    sendto(receiveSocket, sendBuf, 8, 0, (SOCKADDR*)&sendaddr, len);
                    if (ack == 0) {
                        cout << "receive the file" << filename << " successfully" << endl;
                        seq = 0;
                        ack = 1;
                        out.close();
                        k++;
                        break;
                    }
                }
            }
            else {
                sendto(receiveSocket, sendBuf, 8, 0, (SOCKADDR*)&sendaddr, len);
            }
        }
    }
}

int main()
{
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cout << "WSAStartup failed!" << endl << endl;
        return -1;
    }
    cout << "请输入本机IP地址：";
    cin >> receive_ip;
    cout << "请输入本机端口号：";
    cin >> receive_port;
    cout << "请输入路由器IP地址：";
    cin >> send_ip;
    cout << "请输入路由器端口号：";
    cin >> send_port;
    setaddr(&receiveaddr, receive_ip, receive_port);
    setaddr(&sendaddr, send_ip, send_port);
    receiveSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (receiveSocket == INVALID_SOCKET) {
        cout << "Create socket failed！" << endl;
        WSACleanup();
        return -1;
    }
    cout << "Create socket successfully!" << endl;
    if (bind(receiveSocket, (SOCKADDR*)&receiveaddr, sizeof(receiveaddr)) == SOCKET_ERROR) {
        cout << "Bind failed!" << endl;
        closesocket(receiveSocket);
        WSACleanup();
        return -1;
    }
    cout << "Bind successfully!" << endl;
    if (handshake()) {
        recvdata();
    }
    closesocket(receiveSocket);
    WSACleanup();
    return 0;
}
