#ifndef __TEMPORAR_DATA_H__
#define __TEMPORAR_DATA_H__
#include "TcpServer.h"

//暂存列队信息
typedef struct TemporaryData
{
    void *data;
    size_t dataLen;
    int (*syncFun)(struct TemporaryData *, DeviceUnit *);
    int (*deleteMemory)(TemporaryData *);
    std::list<DeviceUnit *> deviceList;
    TemporaryData();
} TemporaryData;

//查找当前key的暂存列队并执行
int GetTemporaryData(PhoneLinkDevice *args, DeviceUnit *device);

//设置当前key的暂存列队
int SetTemporaryData(PhoneLinkDevice *args, DeviceUnit *device, void *data, size_t dataLen, int (*temporaryFun)(TemporaryData *, DeviceUnit *), int (*deleteMemory)(TemporaryData *));

//删除当前key的暂存列队
int deleteTemporaryData(PhoneLinkDevice *args);

#endif
