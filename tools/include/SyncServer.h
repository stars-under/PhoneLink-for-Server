#ifndef __SYNC_SERVER_H__
#define __SYNC_SERVER_H__

#include "TcpServer.h"

typedef struct ShellType
{
    int (*businessFun)(PhoneLinkDevice *args, DeviceUnit *device);
    char *matchKey;
} ShellType;

#endif
