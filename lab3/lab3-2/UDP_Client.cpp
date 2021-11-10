#include <iostream>
#include <fstream>
#include <WinSock2.h>
#include <string>
#include <time.h>
#include <thread>
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
char* sendBuf;
char* resendBuf;
char* buffer;
int GBN_N;//滑动窗口大小
int base = 0;
int nextseqnum = 0;
clock_t start;
clock_t last;
clock_t s;
clock_t l;
int seq;
int ack;
int pnum;
int length;
int totallength = 0;
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

void setPacketHead() {
    sendBuf[0] = 0;//seq
    sendBuf[1] = 0;//ack
    sendBuf[2] = 0;//ACK
    sendBuf[3] = 0;//1 for SYN; 2 for FIN
    sendBuf[4] = 0;//长度高8位
    sendBuf[5] = 0;//长度低8位
    sendBuf[6] = 0;//校验和高8位
    sendBuf[7] = 0;//校验和低8位
}

int handshake() {
    char sendBuf[8] = { 0 };
    char receiveBuf[8] = { 0 };
    seq = rand() % 100;
    sendBuf[0] = seq;//得到随口数j，并将j保存到seq对应位置
    sendBuf[3] = 1;//SYN，表示握手报文
    //将数据报两位两位拼接并进行校验和计算
    u_short buf[5] = { 0 };
    int i;
    for (i = 0;i < 8;i += 2) {
        buf[i / 2] = ((u_char)sendBuf[i] << 8);
        if ((i + 1) < 8) {
            buf[i / 2] += (u_char)sendBuf[i + 1];
        }
    }
    //将校验和填入对应位置
    u_short checks = checksum(buf, i / 2);
    sendBuf[6] = (u_char)(checks >> 8);//校验和
    sendBuf[7] = (u_char)(checks & 0xFF);//校验和
    sendto(sendSocket, sendBuf, 8, 0, (SOCKADDR*)&receiveaddr, len);
    //cout << "send" << endl;
    int res = 0;
    while (true) {
        res = recvfrom(sendSocket, receiveBuf, 8, 0, (SOCKADDR*)&receiveaddr, &len);
        if (res == SOCKET_ERROR) {
            Sleep(2000);
            continue;
        }
        if (receiveBuf[3] == 1 && receiveBuf[2] == 1) {
            //获取确认报文的ack
            ack = receiveBuf[1];
            //如果ack=seq+1，则握手成功
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

void resend(int b, int n) {
    resendBuf = new char[packetsize];
    memset(resendBuf, 0, packetsize);
    for (int i = b;i < n;i++) {
        if (n <= base) {
            break;
        }
        resendBuf[0] = 0;//seq
        resendBuf[1] = 0;//ack
        resendBuf[2] = 0;//ACK
        resendBuf[3] = 0;//SYN
        resendBuf[4] = 0;//长度
        resendBuf[5] = 0;//长度
        resendBuf[6] = 0;//校验和
        resendBuf[7] = 0;//校验和
        int ll = min(packetsize - 8, length - i * (packetsize - 8));
        resendBuf[4] = ll >> 8;//长度
        resendBuf[5] = ll & 0xFF;//长度
        for (int j = 0;j < ll;j++) {
            resendBuf[j + 8] = buffer[i * (packetsize - 8) + j];
        }
        if (i == pnum - 1) {//最后一个包
            resendBuf[1] = 0;
        }
        else {
            resendBuf[1] = 1;
        }
        resendBuf[0] = (u_char)i & 0xFF;
        u_short* buf = new u_short[packetsize / 2 + 1];
        memset(buf, 0, packetsize / 2 + 1);
        int k;
        for (k = 0;k < 8 + ll;k += 2) {
            buf[k / 2] = ((u_char)resendBuf[k] << 8);
            if ((k + 1) < 8 + ll) {
                buf[k / 2] += (u_char)resendBuf[k + 1];
            }
        }
        u_short checks = checksum(buf, k / 2);
        resendBuf[6] = checks >> 8;//校验和
        resendBuf[7] = checks & 0xFF;//校验和
        sendto(sendSocket, resendBuf, packetsize, 0, (SOCKADDR*)&receiveaddr, len);
    }
}

DWORD WINAPI TimerThread(LPVOID lpParameter) {
    while (1) {
        last = clock();
        if (last - start >= 500) {
            start = clock();
            resend(base, nextseqnum);
        }
        if (base == nextseqnum && nextseqnum == pnum) {
            break;
        }
    }
    return 0;
}

DWORD WINAPI RevThread(LPVOID lpParameter) {
    char receiveBuf[8] = { 0 };
    int res = 0;
    while (true) {
        res = recvfrom(sendSocket, receiveBuf, 8, 0, (SOCKADDR*)&receiveaddr, &len);
        if (res == SOCKET_ERROR) {
            Sleep(100);
            continue;
        }
        if (receiveBuf[2] == 1) {//当收到ACK，确认基序号base加1，开启定时器
            base++;
            start = clock();
        }
        //线程结束条件
        if (base == nextseqnum && nextseqnum == pnum) {
            break;
        }
    }
    return 0;
}

void senddata(string filename) {
    base = 0;
    nextseqnum = 0;
    cout << "start to send " << filename << "..." << endl;
    char receiveBuf[8] = { 0 };
    sendBuf = new char[packetsize];
    memset(sendBuf, 0, packetsize);
    ifstream in(filename, ifstream::in | ios::binary);
    in.seekg(0, in.end);//将指针定位在文件结束处
    length = in.tellg();//获取文件大小
    in.seekg(0, in.beg);//将指针定位在文件头
    buffer = new char[length];
    memset(buffer, 0, sizeof(char) * length);
    in.read(buffer, length);//读取文件数据到缓冲区buffer中
    in.close();//关闭文件
    totallength += length;//将该文件长度加入总传输数据长度
    //计算需要传输的包数
    pnum = length / (packetsize - 8) + ((length % (packetsize - 8)) > 0 ? 1 : 0);
    while (true) {
        if (nextseqnum < base + GBN_N && nextseqnum < pnum) {
            setPacketHead();
            //计算数据包长度
            int ll = min((packetsize - 8), length - nextseqnum * (packetsize - 8));
            sendBuf[4] = ll >> 8;//长度
            sendBuf[5] = ll & 0xFF;//长度
            //从缓冲区获取数据
            for (int i = 0;i < ll;i++) {
                sendBuf[i + 8] = buffer[nextseqnum * (packetsize - 8) + i];
            }
            //如果是最后一个包，将ack置为0
            if (nextseqnum == pnum - 1) {
                ack = 0;
            }
            else {
                ack = 1;
            }
            //将nextseqnum与255按位与得到的seq保存到数据报头中
            sendBuf[0] = nextseqnum & 0xFF;
            sendBuf[1] = ack;
            //进行校验和计算
            u_short* buf = new u_short[packetsize / 2 + 1];
            memset(buf, 0, packetsize / 2 + 1);
            int j;
            for (j = 0;j < 8 + ll;j += 2) {
                buf[j / 2] = ((u_char)sendBuf[j] << 8);
                if ((j + 1) < 8 + ll) {
                    buf[j / 2] += (u_char)sendBuf[j + 1];
                }
            }
            u_short checks = checksum(buf, j / 2);
            sendBuf[6] = checks >> 8;//校验和
            sendBuf[7] = checks & 0xFF;//校验和
            //发送数据报
            sendto(sendSocket, sendBuf, packetsize, 0, (SOCKADDR*)&receiveaddr, len);
            if (base == nextseqnum) {//打开定时器
                start = clock();
            }
            nextseqnum++;
        }
        if (nextseqnum == 1) {
            CreateThread(NULL, 0, &TimerThread, NULL, 0, 0);
            CreateThread(NULL, 0, &RevThread, NULL, 0, 0);
        }
        if (base == nextseqnum && nextseqnum == pnum) {
            cout << "send " << filename << " successfully, total " << length << " bytes" << endl;
            break;
        }
    }
}

void handwave() {
    char sendBuf[8] = { 0 };
    char receiveBuf[8] = { 0 };
    seq = rand() % 100;
    sendBuf[0] = seq;
    sendBuf[3] = 2;//FIN
    u_short buf[5] = { 0 };
    int i;
    for (i = 0;i < 8;i += 2) {
        buf[i / 2] = ((u_char)sendBuf[i] << 8);
        if ((i + 1) < 8) {
            buf[i / 2] += (u_char)sendBuf[i + 1];
        }
    }
    u_short checks = checksum(buf, i / 2);
    sendBuf[6] = (u_char)(checks >> 8);//校验和
    sendBuf[7] = (u_char)(checks & 0xFF);//校验和
    //发送握手报文
    sendto(sendSocket, sendBuf, 8, 0, (SOCKADDR*)&receiveaddr, len);
    //cout << "send" << endl;
    int res = 0;
    while (true) {
        res = recvfrom(sendSocket, receiveBuf, 8, 0, (SOCKADDR*)&receiveaddr, &len);
        if (res == SOCKET_ERROR) {
            Sleep(2000);
            continue;
        }
        if (receiveBuf[3] == 2 && receiveBuf[2] == 1) {
            ack = receiveBuf[1];
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
        cout << "start to send data..." << endl; 
        cout << "please choose the size of sliding window: ";
        cin >> GBN_N;
        s = clock();
        senddata("1.jpg");
        senddata("2.jpg");
        senddata("3.jpg");
        senddata("helloworld.txt");
        l = clock();
        double time = (double)(l - s) / CLOCKS_PER_SEC;
        cout << "sendtime:" << time << " s" << endl;
        cout << "totallength:" << totallength << " Bytes" << endl;
        cout << "吞吐率:" << (double)((totallength * 8 / 1000 / 1000) / time) << " Mbps" << endl;
        handwave();
    }
    closesocket(sendSocket);
    WSACleanup();
    return 0;
}
