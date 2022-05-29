#ifndef _TCP_SERVER_H_
#define _TCP_SERVER_H_

#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h> // for sockaddr_in
#include <sys/types.h>  // for socket
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <iostream>
#include <algorithm>
#include <thread>
#include <list>
#include <mutex>
#include <map>

#define errorOut(...) printf(__VA_ARGS__)
#define messageOut(...) printf(__VA_ARGS__)

#define MAX_DATA_LEN (uint32_t)(0x8000000)

#define FunctionErrorDispose(Code, ErrorCode) \
    if ((Code) == 0)                          \
    {                                         \
        ErrorCode;                            \
    }

#define ReadOKErrorDispose(device, OffCode, ErrorCode) \
    switch ((device).ReadOK())                         \
    {                                                  \
    case DeviceOffLine:                                \
        OffCode;                                       \
        break;                                         \
    case NO_OK:                                        \
        ErrorCode;                                     \
        break;                                         \
    }
/*
所以接收的数据应使用该函数释放
*/
void inline DeleteDataMemory(char *&data)
{
    delete[] data;
    data = NULL;
}

//读取设备反馈状态的返回枚举
enum ReadOKStart
{
    DeviceOffLine = 0,
    NO_OK,
    OK
};

// socket数据流类
class socketStream
{
public:
    int socket = NULL;
    socketStream(int socket);
    /**
     * @brief 发送一段数据
     *
     * @param data 指向数据的指针
     * @param len 数据长度
     * @return int 发送正常返回1,失败返回0
     */
    int socketSend(void *data, size_t len);
    /**
     * @brief 从流中读取一段数据
     *
     * @param len 读取数据的长度
     * @return char* 成功返回数据指针,失败返回NULL
     *
     * @note 请在数据不使用时调用DeleteDataMemory函数释放
     */
    char *socketRead(size_t *len);
    /**
     * @brief 发送string(char)字符串
     *
     * @param data 指向字符串的指针
     * @param len 字符串长度,可置为空(自动计算)
     * @return int 发送正常返回1,失败返回0
     */
    int socketSendString(char *data, size_t len = NULL);
    /**
     * @brief 检测回发的状态
     *
     * @return ReadOKStart 返回状态
     */
    ReadOKStart ReadOK();
    ~socketStream();
};

class DeviceInformation
{
public:
    char *ip = NULL;
    char *gatewayIp = NULL;
    char *gaetwayMac = NULL;
    DeviceInformation();
    ~DeviceInformation();
};

class DeviceUnit
{
public:
    bool OnLineStatus = true;
    char *name = NULL;
    size_t nameLen = NULL;
    socketStream *in = NULL;
    socketStream *out = NULL;
    /**
     * @brief 设备信息模块还未完成
     *
     */
    DeviceInformation *information = NULL;
    DeviceUnit(char *name);
    DeviceUnit(DeviceUnit &device);
    DeviceUnit &operator=(const DeviceUnit &DeviceUnit);
    int operator==(DeviceUnit *unit);
    int operator==(DeviceUnit unit);
    /**
     * @brief 查找list中是否存在this
     *
     * @param list 被查询的list表
     * @return std::list<DeviceUnit *>::iterator
     */
    std::list<DeviceUnit *>::iterator ListLookUpThis(std::list<DeviceUnit *> *list);
    /**
     * @brief 将该设备离线
     *
     * @return 返回设备状(离线成功为false)
     */
    bool OffLink();
    ~DeviceUnit();
};

enum StreamDriection
{
    IN = 0,
    OUT = 1,
};

typedef struct PhoneLinkDevice
{
public:
    std::mutex *lock;
    int32_t key = NULL;
    char keyString[0x09];
    std::list<DeviceUnit *> deviceUnitList;
    PhoneLinkDevice();
    PhoneLinkDevice(int32_t key);
    int operator==(PhoneLinkDevice device);
    DeviceUnit *lookUpDevice(DeviceUnit *unit);
    DeviceUnit *addDevice(DeviceUnit *unit);
    /**
     * @brief 销毁一个设备
     *
     * @param unit 被删除的设备
     * @return int 返回0
     */
    int deleteDevice(DeviceUnit *unit);
} PhoneLinkDevice;

class TcpServer
{
private:
    unsigned int port;
    // socket 套接字
    int socketData;
    int (*serverFun)(PhoneLinkDevice *, DeviceUnit *);

    typedef struct ServerFrame
    {
        int socket;
        int (*serverIn)(PhoneLinkDevice *, DeviceUnit *);
        struct DataType
        {
            struct DataKey
            {
                int32_t key;
                int32_t CRC;
                StreamDriection streamDriection;
            };
            struct DataKey head;
            //校验头部数据
            int CheckFrame();
        } data;

        struct ThreadType
        {
            std::list<std::thread *> *threadLocalVector;
            std::thread *thisThread;
        } thread;

        static void serverUnit(TcpServer::ServerFrame *);
    } ServerFrame;

public:
    std::list<std::thread *> threadUnitList;
    std::thread *threadLocal = NULL;
    typedef struct DataHead
    {
        int32_t len;
        int32_t CRC;
        DataHead();
        DataHead(int32_t len);
        ~DataHead();
        int CheckFrame();
    } DataHead;

    struct sockaddr_in socketConfigure;

    TcpServer(unsigned int port, int (*Fun)(PhoneLinkDevice *, DeviceUnit *));

    static void serverThread(TcpServer *);

    ~TcpServer();
};

#endif
