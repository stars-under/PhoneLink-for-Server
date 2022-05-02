#include <stdio.h>
#include <map>
#include "TcpServer.h"
#include "textSync.h"
#include "TemporaryData.h"

int getDeviceInformation(PhoneLinkDevice *args, DeviceUnit *device)
{
    return 0;
}

int setDeviceInformation(PhoneLinkDevice *args, DeviceUnit *device)
{
    struct sockaddr_in in;
    uint32_t inLen = sizeof(in);
    getpeername(device->in->socket, (struct sockaddr *)&in, &inLen);
    char *buff = inet_ntoa(in.sin_addr);
    device->information->ip = new char[strlen(buff)];
    memcpy(device->information->ip, buff, strlen(buff));
    return 0;
}

ShellType funFrom[] = {
    textSync, "textSync",
    GetTemporaryData, "GetTemporaryData"};

int fun(PhoneLinkDevice *args, DeviceUnit *device)
{
    size_t len;

    messageOut("0x%X\n", args->key);

    while (true)
    {
        char *businessStr = device->in->socketRead(&len);
        if (businessStr == NULL)
        {
            //设备断开,将设备离线
            device->OffLink();
            return -1;
        }
        for (size_t i = 0; i < sizeof(funFrom) / sizeof(ShellType); i++)
        {
            if (strncmp(businessStr, funFrom[i].matchKey, len) == 0)
            {
                args->lock->lock();
                int funSign = funFrom[i].businessFun(args, device);
                messageOut("目标函数执行成功,返回值为%d\n", funSign);
                args->lock->unlock();
                goto funEXIT;
            }
        }
        device->in->socketSendString("ON business FUN");
    funEXIT:
        DeleteDataMemory(businessStr);
    }
    return 0;
}

/* Catch Signal Handler functio */
void signal_callback_handler(int signum)
{
    printf("Caught signal SIGPIPE %d\n", signum);
    fflush(stdout);
}

int main()
{
    signal(SIGPIPE, signal_callback_handler);
    printf("hello word!");
    TcpServer server(2562, &fun);
    fflush(stdout);
    if (server.threadLocal != NULL)
    {
        server.threadLocal->join();
    }
    
    printf("时间到达\n");
    return 0;
}
