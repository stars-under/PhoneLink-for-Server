#include "SyncImage.h"
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

#define ToStringUnit(str) #str

#define ToString(str) ToStringUnit(str)

const char Path[] = ToString(SAVE_FILE_PATH);

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

int ImageFile::SaveImage(char *keyPath)
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

ImageFile::~ImageFile()
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

int SyncImage(PhoneLinkDevice *args, DeviceUnit *device)
{
    device->in->socketSendString("OK");

    ImageFile *image = new ImageFile;
    size_t len = 0;

    FunctionErrorDispose(image->name = device->in->socketRead(&len), return -1);

    FunctionErrorDispose(device->in->socketSendString("OK"), return -1);

    FunctionErrorDispose(image->data = device->in->socketRead(&len), return -1);

    image->len = len;

    image->SaveImage(args->keyString);

    int scoketSign;

    int32_t syncSuccessNum = 0;

    for (std::list<DeviceUnit *>::iterator i = args->deviceUnitList.begin(); i != args->deviceUnitList.end(); i++)
    {
        DeviceUnit *deviceTarget = (*i);
        //????????????????????????
        if (deviceTarget->OnLineStatus == false || deviceTarget == device)
        {
            continue;
        }

        //????????????
        scoketSign = deviceTarget->out->socketSendString("SyncImage");
        if (scoketSign <= 0)
        {
            goto DeviceOff;
        }

        switch (deviceTarget->out->ReadOK())
        {
        case NO_OK:
            //??????????????????
            continue;
            break;
        case DeviceOffLine:
            //????????????
            goto DeviceOff;
            break;
        }

        //??????ImageName
        scoketSign = deviceTarget->out->socketSendString(image->name);
        if (scoketSign <= 0)
        {
            goto DeviceOff;
        }

        switch (deviceTarget->out->ReadOK())
        {
        case NO_OK:
            //??????????????????
            continue;
            break;
        case DeviceOffLine:
            //????????????
            goto DeviceOff;
            break;
        }

        //??????ImageData
        scoketSign = deviceTarget->out->socketSend(image->data, image->len);
        if (scoketSign == NULL)
        {
            goto DeviceOff;
        }

        syncSuccessNum++;

        continue;

    DeviceOff:
        messageOut("????????????????????????.???????????????\n");
        args->lock->unlock();
        deviceTarget->OffLink();
        args->lock->lock();
    }

    FunctionErrorDispose(device->in->socketSendString("OK"), return -1);

    FunctionErrorDispose(device->in->socketSend(&syncSuccessNum, sizeof(syncSuccessNum)), return -1);

    SetTemporaryData(args, device, image, sizeof(ImageFile), &ImageTemporary, &ImageDeleteMemory);
    return 0;
}

int ImageTemporary(TemporaryData *data, DeviceUnit *device)
{
    ImageFile *image = (ImageFile *)data->data;

    int scoketSign;
    //????????????
    FunctionErrorDispose(device->out->socketSendString("SyncImage"), return -1);

    ReadOKErrorDispose(*device->out, return -1, return -2);

    FunctionErrorDispose(device->out->socketSendString(image->name), return -1);

    ReadOKErrorDispose(*device->out, return -1, return -2);

    FunctionErrorDispose(device->out->socketSend(image->data, image->len), return -1);

    return 1;
}

int ImageDeleteMemory(TemporaryData *args)
{
    delete (ImageFile *)args->data;
    args->data = NULL;
    return true;
}
