#include "textSync.h"

int DeleteTemporaryDataMomery(TemporaryData *args)
{
    DeleteDataMemory(args->data);
    return 1;
}

int textSync(PhoneLinkDevice *args, DeviceUnit *device)
{
    size_t len = 0;
    int scoketSign = 0;

    FunctionErrorDispose(device->in->socketSendString("OK"), return -1);

    size_t textLen;

    char *text;
    FunctionErrorDispose(text = device->in->socketRead(&textLen),return -1);

    if (text == NULL)
    {
        return 0;
    }

    int32_t syncSuccessNum = 0;

    for (std::list<DeviceUnit *>::iterator i = args->deviceUnitList.begin(); i != args->deviceUnitList.end(); i++)
    {
        DeviceUnit *deviceTarget = (*i);
        //设备离线或是自身
        if (deviceTarget->OnLineStatus == false || deviceTarget == device)
        {
            continue;
        }

        //发送请求
        scoketSign = deviceTarget->out->socketSendString("textSync");

        if (scoketSign <= 0)
        {
            messageOut("检查到故障的设备.已离线设备\n");
            args->lock->unlock();
            deviceTarget->OffLink();
            args->lock->lock();
            continue;
        }

        char *deviceSign = deviceTarget->out->socketRead(&len);

        if (deviceSign == NULL)
        {
            messageOut("检查到故障的设备.已离线设备\n");
            args->lock->unlock();
            deviceTarget->OffLink();
            args->lock->lock();
            continue;
        }

        if (strncmp(deviceSign, "OK", len) != 0)
        {
            deviceTarget->out->socketSend(deviceSign, len);
            errorOut("textSync失败 错误:%s", deviceSign);
            continue;
        }

        DeleteDataMemory(deviceSign);

        scoketSign = deviceTarget->out->socketSend(text, textLen);

        syncSuccessNum++;

        continue;
    }

    //数据在暂存区内生命周期还没结束
    // DeleteDataMemory(text);

    FunctionErrorDispose(device->in->socketSendString("OK"),return -1);

    FunctionErrorDispose(device->in->socketSend(&syncSuccessNum, sizeof(syncSuccessNum)),return -1);

    SetTemporaryData(args, device, text, textLen, textTemporaryDataSync, DeleteTemporaryDataMomery);

    return 1;
}

int textTemporaryDataSync(TemporaryData *args, DeviceUnit *device)
{
    device->out->socketSendString("textSync");
    size_t len;
    char *deviceSign = device->out->socketRead(&len);

    if (deviceSign == NULL)
    {
        return -1;
    }

    if (strncmp(deviceSign, "OK", len) != 0)
    {
        device->out->socketSend(deviceSign, len);
        errorOut("textSync失败 错误:%s", deviceSign);
        return -1;
    }

    DeleteDataMemory(deviceSign);

    device->out->socketSend(args->data, args->dataLen);

    return 1;
}
