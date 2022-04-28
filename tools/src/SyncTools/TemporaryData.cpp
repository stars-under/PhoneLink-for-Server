#include "TemporaryData.h"

std::map<int32_t, TemporaryData *> temporaryData;

char *getStringCopy(char *data)
{
    size_t len = strlen(data);
    char *str = new char[len];
    memcpy(str, data, len);
    return str;
}

int GetTemporaryData(PhoneLinkDevice *args, DeviceUnit *device)
{
    std::map<int32_t, TemporaryData *>::iterator it = temporaryData.find(args->key);
    if (it == temporaryData.end())
    {
        device->in->socketSendString("当前暂存区无数据");
        return 0;
    }
    if (it->second->syncFun == NULL)
    {
        device->in->socketSendString("No Fun");
        errorOut("同步数据无函数!\n");
        return 0;
    }

    std::list<DeviceUnit *>::iterator itDevice = device->ListLookUpThis(it->second->deviceList);

    if (itDevice != it->second->deviceList.end())
    {
        device->in->socketSendString("当前暂存区无数据");
        return 0;
    }

    if (it->second->syncFun(it->second, device) >= 0)
    {

        it->second->deviceList.push_front(device);
        device->in->socketSendString("OK");
        return 1;
    }
    return -1;
}

std::list<char *>::iterator lookUpListCharPointer(std::list<char *> *list, char *str)
{
    for (std::list<char *>::iterator i = list->begin(); i != list->end(); i++)
    {
        if (strcmp(*i, str) == 0)
        {
            return i;
        }
    }
    return list->end();
}

int SetTemporaryData(PhoneLinkDevice *args, DeviceUnit *device, char *funName, char *data, size_t dataLen, int (*temporaryFun)(TemporaryData *, DeviceUnit *))
{
    std::map<int32_t, TemporaryData *>::iterator it = temporaryData.find(args->key);
    if (it == temporaryData.end())
    {
        TemporaryData *data = new TemporaryData;
        std::pair<std::map<int32_t, TemporaryData *>::iterator, bool> res = temporaryData.insert(std::make_pair(args->key, data));
        it = res.first;
    }
    it->second->funName = funName;
    it->second->data = data;
    it->second->dataLen = dataLen;
    it->second->syncFun = temporaryFun;
    for (std::list<DeviceUnit *>::iterator i = args->deviceUnitList.begin(); i != args->deviceUnitList.end(); i++)
    {
        DeviceUnit *deviceTarget = (*i);
        if (deviceTarget == device)
        {
            continue;
        }
        it->second->deviceList.push_back(deviceTarget);
    }
    return true;
}

int deleteTemporaryData(PhoneLinkDevice *args)
{
    std::map<int32_t, TemporaryData *>::iterator it = temporaryData.find(args->key);
    if (it == temporaryData.end())
    {
        return false;
    }
    delete it->second;
    temporaryData.erase(it);
    return true;
}
