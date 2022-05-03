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
    ImageFile image;
    size_t len = 0;
    image.name = device->in->socketRead(&len);
    device->in->socketSendString("OK");
    image.data = device->in->socketRead(&len);
    image.len = len;
    device->in->socketSendString("OK");
    
    image.SaveImage(args->keyString);
    return 0;
}