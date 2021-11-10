#include <iostream>
#include <fstream>
#include <WinSock2.h>
#include <string>
#include <time.h>
#pragma comment(lib,"ws2_32.lib")
using namespace std;
WSADATA wsaData = { 0 };
string send_ip; //发送端IP
u_short send_port; //发送端端口
string receive_ip; //接收端IP
u_short receive_port; //接收端端口
SOCKET sendSocket = INVALID_SOCKET;//发送端套接字
SOCKADDR_IN sendaddr = { 0 }; //发送端地址
SOCKADDR_IN receiveaddr = { 0 }; //接收端地址
int len = sizeof(receiveaddr);
int ack;//确认序号
int seq;//序号
clock_t s;
clock_t l;
const int packetsize = 1500;
int totallength = 0;

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
    sendBuf[0] = 0;//seq高8位
    sendBuf[1] = 0;//seq低8位
    sendBuf[2] = 0;//ack高8位
    sendBuf[3] = 0;//ack低8位
    sendBuf[4] = 0;//ACK
    sendBuf[5] = 0;//SYN/FIN(1 for SYN, 2 for FIN)
    sendBuf[6] = 0;//长度高8位
    sendBuf[7] = 0;//长度低8位
    sendBuf[8] = 0;//校验和高8位
    sendBuf[9] = 0;//校验和低8位
}

int handshake() {
    char sendBuf[10] = { 0 };
    char receiveBuf[10] = { 0 };
    //设置数据报头
    setPacketHead(sendBuf);
    seq = rand() % 100;//得到随机数j，并将j保存到seq对应位置
    sendBuf[0] = (u_char)(seq>>8);
    sendBuf[1] = (u_char)(seq & 0xFF);
    sendBuf[5] = 1;//SYN，表示握手报文
    //使用u_short数组将char型数据包两位两位拼接并进行校验和
    u_short buf[6] = { 0 };
    int i;
    for (i = 0;i < 10;i += 2) {
        buf[i / 2] = ((u_char)sendBuf[i] << 8);
        if ((i + 1) < 10) {
            buf[i / 2] += (u_char)sendBuf[i + 1];
        }
    }
    u_short checks = checksum(buf, i / 2);
    //将校验和填入对应位置
    sendBuf[8] = (u_char)(checks >> 8);//校验和
    sendBuf[9] = (u_char)(checks & 0xFF);//校验和
    //发送握手请求报文
    sendto(sendSocket, sendBuf, 10, 0, (SOCKADDR*)&receiveaddr, len);
    //cout << "send" << endl;
    int res = 0;
    while (true) {
        res = recvfrom(sendSocket, receiveBuf, 10, 0, (SOCKADDR*)&receiveaddr, &len);
        if (res == SOCKET_ERROR) {
            Sleep(2000);
            continue;
        }
        if (receiveBuf[5] == 1 && receiveBuf[4] == 1) {
            //获取确认报文的ack
            ack = ((u_char)receiveBuf[2] << 8) + (u_char)receiveBuf[3];
            //如果ack=seq+1,则握手成功
            if (seq + 1 == ack) {
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

void senddata(string filename) {
    cout << "start to send " << filename << "..." << endl;
    char receiveBuf[10] = { 0 };
    char* sendBuf = new char[packetsize];
    memset(sendBuf, 0, packetsize);
    seq = 0;
    ifstream in(filename, ifstream::in | ios::binary);
    in.seekg(0, in.end);//将指针定位在文件结束处
    int length = in.tellg();//获取文件大小
    in.seekg(0, in.beg);//将指针定位在文件头
    char* buffer = new char[length];
    memset(buffer, 0, sizeof(char) * length);
    in.read(buffer, length);//读取文件数据到缓冲区buffer中
    in.close();//关闭文件
    totallength += length;
    int ll = 0, lk = 0, resend = 0;
    //循环发送
    while (true) {
        //使用resend标志确认是否为重传
        //如果是重传，不需要处理数据包，直接重新发送一遍
        //如果不是重传，需要处理数据包
        if (!resend) {
            setPacketHead(sendBuf);
            //如果数据缓冲区剩下的数据不足一个数据包
            //那么只读取length-ll的数据，到结束
            lk = min(length - ll, packetsize - 10) + ll;
            //将缓冲区对应段数据放入数据报对应位置
            for (int i = ll;i < lk;i++) {
                sendBuf[i - ll + 10] = (u_char)buffer[i];
            }
            //保存长度
            sendBuf[6] = (lk - ll) >> 8;//长度
            sendBuf[7] = (lk - ll) & 0xFF;//长度
            //保存数据包序列号
            sendBuf[0] = (u_char)(seq >> 8);
            sendBuf[1] = (u_char)(seq & 0xFF);
            //如果之后还有包，将下一个包的序列号保存在ack
            ack = seq + 1;
            //如果是最后一个包，则将ack置0
            if (length - ll <= (packetsize - 10)) {
                ack = 0;
            }
            sendBuf[2] = (u_char)(ack >> 8);
            sendBuf[3] = (u_char)(ack & 0xFF);
            //将数据包两位两位拼接进行校验
            u_short* buf = new u_short[packetsize / 2 + 1];
            memset(buf, 0, packetsize / 2 + 1);
            int i;
            for (i = 0;i < 10 + lk - ll;i += 2) {
                buf[i / 2] = ((u_char)sendBuf[i] << 8);
                if ((i + 1) < 10 + lk - ll) {
                    buf[i / 2] += (u_char)sendBuf[i + 1];
                }
            }
            u_short checks = checksum(buf, i / 2);
            //将校验和字段保存
            sendBuf[8] = checks >> 8;//校验和
            sendBuf[9] = checks & 0xFF;//校验和
        }
        //发送数据包
        sendto(sendSocket, sendBuf, packetsize, 0, (SOCKADDR*)&receiveaddr, len);
        resend = 0;
        //cout << "send " << lk - ll << " bytes data of " << seq << endl;
        int res = 0;
        while (true) {
            res = recvfrom(sendSocket, receiveBuf, 10, 0, (SOCKADDR*)&receiveaddr, &len);
            if (res == SOCKET_ERROR) {
                Sleep(2000);
                continue;
            }
            if (receiveBuf[0] == sendBuf[0] && receiveBuf[1] == sendBuf[1]) {
                if (receiveBuf[4] == 1) {
                    seq++;
                    break;
                }
                else {//ACK=0即NACK，重传
                    resend = 1;
                    break;
                }
            }
        }
        if (!resend) {
            ll = lk;
        }
        if (length - ll <= 0 && (!resend)) {
            break;
        }
    }
    cout << "send " << filename << " successfully, total " << length << " bytes" << endl;
}

void wavehand() {
    char sendBuf[10] = { 0 };
    char receiveBuf[10] = { 0 };
    seq = rand() % 100;
    sendBuf[0] = (u_char)(seq >> 8);
    sendBuf[1] = (u_char)(seq & 0xFF);
    sendBuf[5] = 2;//FIN
    u_short buf[6] = { 0 };
    int i;
    for (i = 0;i < 10;i += 2) {
        buf[i / 2] = ((u_char)sendBuf[i] << 8);
        if ((i + 1) < 10) {
            buf[i / 2] += (u_char)sendBuf[i + 1];
        }
    }
    u_short checks = checksum(buf, i / 2);
    sendBuf[8] = (u_char)(checks >> 8);//校验和
    sendBuf[9] = (u_char)(checks & 0xFF);//校验和
    sendto(sendSocket, sendBuf, 10, 0, (SOCKADDR*)&receiveaddr, len);
    //cout << "send" << endl;
    int res = 0;
    while (true) {
        res = recvfrom(sendSocket, receiveBuf, 10, 0, (SOCKADDR*)&receiveaddr, &len);
        if (res == SOCKET_ERROR) {
            Sleep(2000);
            continue;
        }
        if (receiveBuf[5] == 2 && receiveBuf[4] == 1) {
            ack = ((u_char)receiveBuf[2] << 8) + (u_char)receiveBuf[3];
            if (seq + 1 == ack) {
                cout << "wave hand successfully" << endl;
            }
            else {
                cout << "wave hand failed" << endl;
            }
            break;
        }
    }
}

int main()
{
    srand((unsigned)time(NULL));
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) { 
        cout << "WSAStartup failed!" << endl << endl;
        return -1;
    }
    cout << "请输入本机IP地址：";
    cin >> send_ip;
    cout << "请输入本机端口号：";
    cin >> send_port;
    cout << "请输入服务端IP地址：";
    cin >> receive_ip;
    cout << "请输入服务端端口号：";
    cin >> receive_port;
    setaddr(&sendaddr, send_ip, send_port);
    setaddr(&receiveaddr, receive_ip, receive_port);
    sendSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (sendSocket == INVALID_SOCKET) {
        cout << "Create socket failed！" << endl;
        WSACleanup();
        return -1;
    }
    cout << "Create socket successfully!" << endl;
    if (bind(sendSocket, (SOCKADDR*)&sendaddr, sizeof(sendaddr)) == SOCKET_ERROR) {
        cout << "Bind failed!" << endl;
        closesocket(sendSocket);
        WSACleanup();
        return -1;
    }
    cout << "Bind successfully!" << endl;
    if (handshake()) {
        s = clock();
        senddata("1.jpg");
        senddata("2.jpg");
        senddata("3.jpg");
        senddata("helloworld.txt");
        wavehand();
        l = clock();
        double time = (double)(l - s) / CLOCKS_PER_SEC;
        cout << "sendtime:" << time << " s" << endl;
        cout << "totallength:" << totallength << " Bytes" << endl;
        cout << "吞吐率:" << (double)((totallength * 8 / 1000) / time) << " Kbps" << endl;
    }
    closesocket(sendSocket);
    WSACleanup();
    return 0;
}
