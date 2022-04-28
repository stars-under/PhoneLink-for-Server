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

#define MAX_DATA_LEN (0x10000)

#define DeleteDataMemory(data) \
    delete[] data;             \
    data = NULL

class socketStream
{
public:
    int socket = NULL;
    socketStream(int socket);
    int socketSend(void *data, size_t len);
    char *socketRead(size_t *len);
    int socketSendString(char *data, size_t len = NULL);
    ~socketStream();
};

class DeviceInformation
{
public:
    char *ip = NULL;
    char *gatewayIp = NULL;
    char *gaetwayMac = NULL;
    DeviceInformation(/* args */);
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
    DeviceInformation *information = NULL;
    DeviceUnit(char *name);
    DeviceUnit(DeviceUnit &device);
    DeviceUnit &operator=(const DeviceUnit &DeviceUnit);
    int operator==(DeviceUnit *unit);
    int operator==(DeviceUnit unit);
    std::list<DeviceUnit*>::iterator ListLookUpThis(std::list<DeviceUnit*> *list);
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
    std::list<DeviceUnit *> deviceUnitList;
    PhoneLinkDevice();
    PhoneLinkDevice(int32_t key);
    int operator==(PhoneLinkDevice device);
    DeviceUnit *lookUpDevice(DeviceUnit *unit);
    DeviceUnit *addDevice(DeviceUnit *unit);
    int deleteDevice(DeviceUnit *unit);
} PhoneLinkDevice;

class TcpServer
{
private:
    unsigned int port;
    int socketData;
    int serverState;
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
        } Data;

        struct
        {
            std::list<std::thread *> *threadLocalVector;
            std::thread *thisThread;
        };

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
