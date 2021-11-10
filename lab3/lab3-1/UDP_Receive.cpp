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
SOCKADDR_IN receiveaddr = { 0 }; //接收端地址
SOCKADDR_IN sendaddr = { 0 }; //发送端地址
int len = sizeof(sendaddr);
int seq;//序号
int ack;//确认序号
char filename;//文件名
const int packetsize = 1500;

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
    sendBuf[1] = 0;//seq
    sendBuf[2] = 0;//ack
    sendBuf[3] = 0;//ack
    sendBuf[4] = 0;//ACK
    sendBuf[5] = 0;//SYN
    sendBuf[6] = 0;//长度
    sendBuf[7] = 0;//长度
    sendBuf[8] = 0;//校验和
    sendBuf[9] = 0;//校验和
}

int handshake() {
    char sendBuf[10] = { 0 };
    char receiveBuf[10] = { 0 };
    int res = 0;
    while (true) {
        //接收报文
        res = recvfrom(receiveSocket, receiveBuf, 10, 0, (SOCKADDR*)&sendaddr, &len);
        if (res == SOCKET_ERROR) {
            Sleep(2000);
            continue;
        }
        if (receiveBuf[5] == 1) {//即数据报的SYN位为1，标志握手请求报文
            //设置发送数据报头
            setPacketHead(sendBuf);
            //两位两位拼接进行校验和
            u_short buf[6] = { 0 };
            int i;
            for (i = 0;i < 10;i += 2) {
                buf[i / 2] = (u_char)receiveBuf[i] << 8;
                if ((i + 1) < 10) {
                    buf[i / 2] += (u_char)receiveBuf[i + 1];
                }
            }
            u_short checks = checksum(buf, i / 2);
            if (checks == 0) {//校验和为0，表示数据报传输正确
                //获取数据报中的seq
                seq = ((u_char)receiveBuf[0] << 8) + (u_char)receiveBuf[1];
                sendBuf[0] = (u_char)(seq >> 8);
                sendBuf[1] = (u_char)(seq & 0xFF);
                //确认序号ack=seq+1
                ack = seq + 1;
                sendBuf[2] = (u_char)(ack >> 8);
                sendBuf[3] = (u_char)(ack & 0xFF);
                //标志位SYN和ACK置1
                sendBuf[4] = 1;//ACK
                sendBuf[5] = 1;//SYN
                sendBuf[8] = checks >> 8;//校验和
                sendBuf[9] = checks & 0xFF;//校验和
                //发送握手确认报文
                sendto(receiveSocket, sendBuf, 10, 0, (SOCKADDR*)&sendaddr, len);
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
    seq = -1;
    int k = 1;
    char sendBuf[10] = { 0 };
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
            //数据报的第6个字节为2，表示挥手报文
            if (receiveBuf[5] == 2) {
                setPacketHead(sendBuf);
                u_short buf[6] = { 0 };
                int i;
                for (i = 0;i < 10;i += 2) {
                    buf[i / 2] = (u_char)receiveBuf[i] << 8;
                    if ((i + 1) < 10) {
                        buf[i / 2] += (u_char)receiveBuf[i + 1];
                    }
                }
                u_short checks = checksum(buf, i / 2);
                if (checks == 0) {
                    seq = ((u_char)receiveBuf[0] << 8) + (u_char)receiveBuf[1];
                    sendBuf[0] = (u_char)(seq >> 8);
                    sendBuf[1] = (u_char)(seq & 0xFF);
                    ack = seq + 1;
                    sendBuf[2] = (u_char)(ack >> 8);
                    sendBuf[3] = (u_char)(ack & 0xFF);
                    sendBuf[4] = 1;//ACK
                    sendBuf[5] = 2;//FIN
                    sendBuf[8] = checks >> 8;//校验和
                    sendBuf[9] = checks & 0xFF;//校验和
                    sendto(receiveSocket, sendBuf, 10, 0, (SOCKADDR*)&sendaddr, len);
                    cout << "wave hand successfully" << endl;
                    return;
                }
                else {
                    cout << "wave hand failed" << endl;
                    return;
                }
            }
            if (receiveBuf[5] != 2 && receiveBuf[5] != 1 && //数据报的第6个字节表示SYN(1)/FIN(2)
                seq + 1 == ((u_char)receiveBuf[0] << 8) + (u_char)receiveBuf[1]) {//确认数据包序号连续
                ofstream out(&filename, ifstream::out | ios::binary | ios::app);
                setPacketHead(sendBuf);
                u_short* buf = new u_short[packetsize / 2 + 1];
                memset(buf, 0, packetsize / 2 + 1);
                //获取数据的长度，数据报的序列号和下一个序列号
                int length = ((u_char)receiveBuf[6] << 8) + (u_char)receiveBuf[7];
                seq = ((u_char)receiveBuf[0] << 8) + (u_char)receiveBuf[1];
                ack = ((u_char)receiveBuf[2] << 8) + (u_char)receiveBuf[3];
                //计算校验和
                int i;
                for (i = 0;i < 10 + length;i += 2) {
                    buf[i / 2] = ((u_char)receiveBuf[i] << 8);
                    if ((i + 1) < 10 + length) {
                        buf[i / 2] += (u_char)receiveBuf[i + 1];
                    }
                }
                u_short checks = checksum(buf, i / 2);
                sendBuf[0] = receiveBuf[0];
                sendBuf[1] = receiveBuf[1];
                sendBuf[2] = receiveBuf[2];
                sendBuf[3] = receiveBuf[3];
                sendBuf[4] = 0;//ACK
                sendBuf[8] = checks >> 8;//校验和
                sendBuf[9] = checks & 0xFF;//校验和
                if (checks == 0) {
                    sendBuf[4] = 1;//ACK
                    char* buffer = new char[length];
                    for (int j = 0;j < length;j++) {
                        buffer[j] = receiveBuf[10 + j];
                    }
                    out.write(buffer, length);
                }
                sendto(receiveSocket, sendBuf, 10, 0, (SOCKADDR*)&sendaddr, len);
                if (checks == 0 && ack == 0) {
                    cout << "receive the file" << filename << " successfully" << endl;
                    seq = -1;
                    out.close();
                    k++;
                    break;
                }
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
    setaddr(&receiveaddr, receive_ip, receive_port);
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