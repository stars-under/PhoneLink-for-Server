#include "SyncImage.h"
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

const char Path[] = SAVE_FILE_PATH;

#define DeleteStringMomery(momery) delete[] momery

char *MeargeCharPointer(const char *str1, const char *str2)
{
    char *str = new char[strlen(str1) + strlen(str2) + 1];
    strcpy(str, str1);
    strcat(str, str2);
    return str;
}

char *MeargeCharPointer(const char *str1, const char *str2, const char *str3)
{
    char *str = new char[strlen(str1) + strlen(str2) + strlen(str3) + 1];
    strcpy(str, str1);
    strcat(str, str2);
    strcat(str, str3);
    return str;
}

char *DirStringMearge(const char *str1, const char *str2)
{
    char *str = new char[strlen(str1) + 1 + strlen(str2) + 1 + 1];
    strcpy(str, str1);
    strcat(str, "/");
    strcat(str, str2);
    strcat(str, "/");
    return str;
}

class ImageFile
{
public:
    char *name;
    size_t len;
    char *data;
    int SaveImage(char *keyPath)
    {
        char *imagePath = DirStringMearge(Path, keyPath);

        if (access(imagePath, 0) == -1)
        {
            int error = mkdir(imagePath, 0777);
            int errors = errno;
        }

        char *imageFilePath = MeargeCharPointer(imagePath, name);
        
        DeleteDataMemory(imagePath);

        FILE *fp = fopen(imageFilePath, "wb");

        DeleteStringMomery(imageFilePath);

        if (fp == NULL)
        {
            return -1;
        }

        fwrite(data, sizeof(char), len, fp);

        fclose(fp);

        return 0;
    }
    ~ImageFile()
    {
        if (name != NULL)
        {
            DeleteDataMemory(name);
        }
        if (data != NULL)
        {
            DeleteDataMemory(data);
        }
    }
};

int SyncImage(PhoneLinkDevice *args, DeviceUnit *device)
{
    device->in->socketSendString("OK");

    ImageFile *image = new ImageFile;
    size_t len = 0;

    image->name = device->in->socketRead(&len);
    device->in->socketSendString("OK");
    image->data = device->in->socketRead(&len);
    image->len = len;
    device->in->socketSendString("OK");

    image->SaveImage(args->keyString);
    
    int scoketSign;

    size_t syncSuccessNum = 0;
    
    for (std::list<DeviceUnit *>::iterator i = args->deviceUnitList.begin(); i != args->deviceUnitList.end(); i++)
    {
        DeviceUnit *deviceTarget = (*i);
        //设备离线或是自身
        if (deviceTarget->OnLineStatus == false || deviceTarget == device)
        {
            continue;
        }

        //发送请求
        scoketSign = deviceTarget->out->socketSendString("SyncImage");
        if (scoketSign <= 0)
        {
            messageOut("检查到故障的设备.已离线设备\n");
            args->lock->unlock();
            deviceTarget->OffLink();
            args->lock->lock();
            continue;
        }
        
        //校验OK
        if (deviceTarget->out->ReadOK() <= 0)
        {
            //设备出错,将其离线
            args->lock->unlock();
            deviceTarget->OffLink();
            args->lock->lock();
        }
        
        //发送ImageName
        scoketSign = deviceTarget->out->socketSendString(image->name);
        if (scoketSign <= 0)
        {
            messageOut("检查到故障的设备.已离线设备\n");
            args->lock->unlock();
            deviceTarget->OffLink();
            args->lock->lock();
            continue;
        }

        //校验OK
        if (deviceTarget->out->ReadOK() <= 0)
        {
            //设备出错,将其离线
            args->lock->unlock();
            deviceTarget->OffLink();
            args->lock->lock();
        }
        
        //发送ImageData
        scoketSign = deviceTarget->out->socketSend(image->data,image->len);
        if (scoketSign == NULL)
        {
            messageOut("检查到故障的设备.已离线设备\n");
            args->lock->unlock();
            deviceTarget->OffLink();
            args->lock->lock();
            continue;
        }

        syncSuccessNum++;
    }

    return 0;
}