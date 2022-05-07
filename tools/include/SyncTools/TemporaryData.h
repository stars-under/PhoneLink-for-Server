#ifndef __TEMPORAR_DATA_H__
#define __TEMPORAR_DATA_H__
#include "TcpServer.h"

//暂存列队信息
typedef struct TemporaryData
{
    char *data;
    size_t dataLen;
    int (*syncFun)(struct TemporaryData *, DeviceUnit *);
    int (*deleteMemory)(TemporaryData *);
    std::list<DeviceUnit *> deviceList;
    TemporaryData();
} TemporaryData;

//获得一个字符串的副本,请注意释放
char *getStringCopy(char *data);

//将字符串副本释放的宏
#define deleteStringCopy(data) \
    delete data;               \
    data = NULL

//查找当前key的暂存列队并执行
int GetTemporaryData(PhoneLinkDevice *args, DeviceUnit *device);

//查找char*列队中的字符串
std::list<char *>::iterator lookUpListCharPointer(std::list<char *> *list, char *str);

//设置当前key的暂存列队
int SetTemporaryData(PhoneLinkDevice *args, DeviceUnit *device, char *data, size_t dataLen, int (*temporaryFun)(TemporaryData *, DeviceUnit *), int (*deleteMemory)(TemporaryData *));

//删除当前key的暂存列队
int deleteTemporaryData(PhoneLinkDevice *args);

#endif
