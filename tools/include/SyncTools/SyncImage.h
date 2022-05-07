#ifndef __SYNC_IMAGE_H__
#define __SYNC_IMAGE_H__

#include "TcpServer.h"
#include "TemporaryData.h"

class ImageFile
{
public:
    char *name;
    size_t len;
    char *data;
    int SaveImage(char *keyPath);
    ~ImageFile();
};

int SyncImage(PhoneLinkDevice *args, DeviceUnit *device);


#endif
