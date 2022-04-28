#ifndef __TEXT_SYNC_H__
#define __TEXT_SYNC_H__

#include <stdio.h>
#include "SyncServer.h"
#include "TemporaryData.h"

int textTemporaryDataSync(TemporaryData *args, DeviceUnit *device);

int textSync(PhoneLinkDevice *args, DeviceUnit *device);

#endif