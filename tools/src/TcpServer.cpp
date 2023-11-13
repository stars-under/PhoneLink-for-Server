#include "TcpServer.h"

std::list<PhoneLinkDevice> deviceList;

TcpServer::TcpServer(unsigned int port, int (*Fun)(PhoneLinkDevice *, DeviceUnit *))
{
    int serverState;
    
    memset(&this->socketConfigure, 0, sizeof(this->socketConfigure));
    this->socketConfigure.sin_family = AF_INET;
    this->socketConfigure.sin_addr.s_addr = htonl(INADDR_ANY);
    this->socketConfigure.sin_port = htons(port);

    this->socketData = socket(PF_INET, SOCK_STREAM, 0);
    if (this->socketData < 0)
    {
        errorOut("打开socket失败!");
        return;
    }

    //心跳包开启
    int keepAlive = 1;
    if (setsockopt(this->socketData, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof(keepAlive)) < 0)
    {
        errorOut("心跳包设置失败");
        return;
    }

    //连接空闲60秒后发送心跳包
    int keepIdle = 60;
    if (setsockopt(this->socketData, SOL_TCP, TCP_KEEPIDLE, &keepIdle, sizeof(keepIdle)) < 0)
    {
        errorOut("心跳包设置失败");
        return;
    }

    //心跳包之间的间隔
    int keepInterval = 30;
    if (setsockopt(this->socketData, SOL_TCP, TCP_KEEPINTVL, (void *)&keepInterval, sizeof(keepInterval)) < 0)
    {
        errorOut("心跳包设置失败");
        return;
    }

    //最大心跳包数量
    int keepCount = 3;
    if (setsockopt(this->socketData, SOL_TCP, TCP_KEEPCNT, (void *)&keepCount, sizeof(keepCount)) < 0)
    {
        errorOut("心跳包设置失败");
        return;
    }
    int syncnt = 1;
    if (setsockopt(this->socketData, IPPROTO_TCP, TCP_SYNCNT, &syncnt, sizeof(syncnt)) < 0)
    {
        errorOut("SYN设置失败");
        return;
    }

    serverState = bind(this->socketData, (struct sockaddr *)&this->socketConfigure, sizeof(this->socketConfigure));
    if (serverState < 0)
    {
        errorOut("绑定端口失败,错误码:%d\n", serverState);
        errorOut("error is %d", errno);
        return;
    }

    serverState = listen(this->socketData, 10);
    if (serverState < 0)
    {
        errorOut("设置列队失败,错误码:%d", serverState);
        return;
    }

    messageOut("任务创建成功");

    this->serverFun = Fun;

    this->threadLocal = new std::thread(serverThread, this);

    return;
}

TcpServer::~TcpServer()
{
}

void TcpServer::ServerFrame::serverUnit(TcpServer::ServerFrame *data)
{
    do
    {

        memset(&data->data, 0, sizeof(data->data));
        // memset(&buff,NULL,sizeof(buff));
        //将Data本身作为一个对象接收数据
        if (recv(data->socket, &data->data, (sizeof(TcpServer::ServerFrame::DataType::DataKey)), MSG_WAITALL) != sizeof(TcpServer::ServerFrame::DataType::DataKey))
        {
            errorOut("握手包接收错误");
            goto GOTO_EXIT;
        }

        if (data->data.CheckFrame() == false)
        {
            errorOut("Key检验失败");
            goto GOTO_EXIT;
        }

        DataHead head(3);
        send(data->socket, &head, sizeof(head), 0);
        send(data->socket, "OK", strlen("OK") + 1, 0);

        if (data->serverIn == NULL)
        {
            errorOut("回调函数为NULL\n");
            goto GOTO_EXIT;
        }

        PhoneLinkDevice device(data->data.head.key);
        //查找链表中有无该key
        std::list<PhoneLinkDevice>::iterator it = std::find(deviceList.begin(), deviceList.end(), device);

        if (it == deviceList.end())
        {
            //没有改key则添加到list中
            deviceList.push_front(device);
            it = std::find(deviceList.begin(), deviceList.end(), device);
        }
        PhoneLinkDevice *args = &*it;

        socketStream *stream = new socketStream(data->socket);
        size_t len;

        char *deviceName = stream->socketRead(&len);

        if (deviceName == NULL)
        {
            errorOut("接收name时意外的断开\n");
            goto GOTO_EXIT;
        }

        DeviceUnit *deviceUnit = new DeviceUnit(deviceName);
        DeleteDataMemory(deviceName);
        //查找list是否存在该设备
        DeviceUnit *deviceUnitTarget = args->lookUpDevice(deviceUnit);
        if (deviceUnitTarget != NULL)
        {
            //存在可以将新申请的副本释放了
            delete deviceUnit;
            deviceUnit = NULL;

            //检查设备是否离线
            if (deviceUnitTarget->OnLineStatus == false)
            {
                //不存在In端口不允许Out端口连接
                if (data->data.head.streamDriection == OUT)
                {
                    stream->socketSendString((char*)"不存在的IN端口");
                    errorOut("不存在IN端口的OUT端口连接\n");
                    delete stream;
                    goto GOTO_EXIT;
                }

                //离线转在线
                deviceUnitTarget->OnLineStatus = true;
            }
            else
            {
                //设备在线着检查是否为IN端口
                if (data->data.head.streamDriection == IN)
                {
                    //离线原设备
                    deviceUnitTarget->OffLink();
                    //转在线
                    deviceUnitTarget->OnLineStatus = true;
                }
            }
        }
        else
        {
            //不存在In端口不允许Out端口连接
            if (data->data.head.streamDriection == OUT)
            {
                stream->socketSendString((char*)"不存在的IN端口");
                errorOut("不存在IN端口的OUT端口连接\n");
                delete stream;
                goto GOTO_EXIT;
            }

            //不存在则添加
            deviceUnitTarget = args->addDevice(deviceUnit);
        }

        stream->socketSendString((char*)"OK");

        *((data->data.head.streamDriection == IN) ? (&deviceUnitTarget->in) : (&deviceUnitTarget->out)) = stream;

        //端口为OUT时对socket做超时设置
        if (data->data.head.streamDriection == OUT)
        {
            struct timeval timeout = {3, 0}; // 3s
            if (setsockopt(stream->socket, SOL_SOCKET, SO_SNDTIMEO, (const char *)&timeout, sizeof(timeout)) < 0)
            {
                errorOut("SO_SNDTIMEO设置失败");
                return;
            }

            if (setsockopt(stream->socket, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout)) < 0)
            {
                errorOut("SO_RCVTIMEO设置失败");
                return;
            }
        }

        (data->data.head.streamDriection == IN) ? (data->serverIn(&*it, deviceUnitTarget)) : (0);

    } while (false);
GOTO_EXIT:
    uint32_t overTime = 0;
    //销毁socket的任务被socketStream类接管了
    // close(data->socket);
    while (data->thread.thisThread == NULL)
    {
        sleep(1);
        if (++overTime >= 100)
        {
            errorOut("错误:接收线程结束时检测到线程句柄为空");
            delete data;
            return;
        }
    }
    //从<Lits>中删除自身
    data->thread.threadLocalVector->remove(data->thread.thisThread);
    //销毁本地对象
    delete data;
    return;
}

void TcpServer::serverThread(TcpServer *data)
{
    while (true)
    {
        socklen_t len = 0;
        int newSocket = accept(data->socketData, (struct sockaddr *)&data->socketConfigure, &len);
        if (newSocket < 0)
        {
            errorOut("等待连接失败,错误码:%d", newSocket);
            return;
        }

        ServerFrame *frameThread = new ServerFrame();
        frameThread->socket = newSocket;
        frameThread->thread.threadLocalVector = &data->threadUnitList;
        frameThread->serverIn = data->serverFun;

        std::thread *t = new std::thread(ServerFrame::serverUnit, frameThread);
        data->threadUnitList.push_front(t);
        frameThread->thread.thisThread = t;
    }
}

int TcpServer::ServerFrame::DataType::CheckFrame()
{
    if (this->head.key == (~this->head.CRC))
    {
        return true;
    }
    return false;
}

int TcpServer::DataHead::CheckFrame()
{
    if (this->len == (~this->CRC))
    {
        return true;
    }
    return false;
}

TcpServer::DataHead::DataHead(int32_t len)
{
    this->len = len;
    this->CRC = ~len;
}

TcpServer::DataHead::DataHead() {}

int socketStream::socketSend(void *data, size_t len)
{
    if (data == NULL)
    {
        return 0;
    }
    if (len == 0)
    {
        return 0;
    }

    TcpServer::DataHead head(len);
    if (send(this->socket, &head, sizeof(TcpServer::DataHead), 0) <= 0)
    {
        return 0;
    }

    size_t sendLen = 0;
    while (len != 0)
    {
        sendLen = send(this->socket, (char*)data, len, MSG_NOSIGNAL);
        if (sendLen <= 0)
        {
            return 0;
        }
        data = (char*)data + sendLen;
        len -= sendLen;
    }

    return 1;
}

int socketStream::socketSendString(char *data, size_t len)
{
    if (data == NULL)
    {
        return 0;
    }
    if (len == 0)
    {
        len = strlen(data) + 1;
    }

    return this->socketSend(data, len);
}

ReadOKStart socketStream::ReadOK()
{
    size_t len;
    char *deviceSign = this->socketRead(&len);
    if (deviceSign == NULL)
    {
        messageOut("检查到故障的设备.已离线设备\n");
        return DeviceOffLine;
    }
    if (strncmp(deviceSign, "OK", len) != 0)
    {
        errorOut("执行函数失败 错误:%s", deviceSign);
        return NO_OK;
    }
    DeleteDataMemory(deviceSign);
    return OK;
}

char *socketStream::socketRead(size_t *len)
{
    TcpServer::DataHead head;
    int recvSign = recv(this->socket, &head, sizeof(TcpServer::DataHead), MSG_WAITALL);
    if (recvSign <= 0)
    {
        errorOut("连接已断开!!");
        return 0;
    }
    if (!head.CheckFrame())
    {
        errorOut("数据长度(%d)校验错误!!", head.len);
        return 0;
    }
    if (head.len >= MAX_DATA_LEN)
    {
        errorOut("数据长度(%d)过大!!", head.len);
        return 0;
    }

    char *data = new char[head.len];

    int scoketSign = 0;

    scoketSign = recv(this->socket, data, head.len, MSG_WAITALL);

    if (scoketSign != head.len)
    {
        errorOut("接收数据长度出错,预计长度%d,实际返回%d", head.len, scoketSign);
        return 0;
    }

    *len = head.len;

    return data;
}

TcpServer::DataHead::~DataHead()
{
}

DeviceUnit::DeviceUnit(char *name)
{
    size_t len = strlen(name) + 1;
    this->name = new char[len];
    this->nameLen = len;
    memcpy(this->name, name, len);
}

DeviceUnit::DeviceUnit(DeviceUnit &device)
{
    this->name = new char[device.nameLen];
    memcpy(this->name, device.name, device.nameLen);
    this->nameLen = device.nameLen;
}

DeviceUnit &DeviceUnit::operator=(const DeviceUnit &device)
{
    this->name = new char[device.nameLen];
    memcpy(this->name, device.name, device.nameLen);
    this->nameLen = device.nameLen;
    return *this;
}

bool DeviceUnit::OffLink()
{
    this->OnLineStatus = false;
    if (this->in != NULL)
    {
        delete this->in;
        this->in = NULL;
    }
    if (this->out != NULL)
    {
        delete this->out;
        this->out = NULL;
    }
    return false;
}

DeviceUnit::~DeviceUnit()
{
    if (this->name != NULL)
    {
        delete[] this->name;
        this->name = NULL;
    }
    if (this->in != NULL)
    {
        delete this->in;
    }
    if (this->out != NULL)
    {
        delete this->out;
    }
}

int DeviceUnit::operator==(DeviceUnit *unit)
{
    if (this == unit)
    {
        return true;
    }
    return false;
}

int DeviceUnit::operator==(DeviceUnit unit)
{
    if (unit.name == this->name)
    {
        return true;
    }

    if (strncmp(unit.name, this->name, this->nameLen) == 0)
    {
        return true;
    }
    return false;
}

std::list<DeviceUnit *>::iterator DeviceUnit::ListLookUpThis(std::list<DeviceUnit *> *list)
{
    for (std::list<DeviceUnit *>::iterator i = list->begin(); i != list->end(); i++)
    {
        if (**i == *this)
        {
            return i;
        }
    }
    return list->end();
}

socketStream::socketStream(int socket)
{
    this->socket = socket;
}

socketStream::~socketStream()
{
    if (this->socket != 0)
    {
        close(this->socket);
    }
}

PhoneLinkDevice::PhoneLinkDevice()
{
    this->lock = new std::mutex();
}

PhoneLinkDevice::PhoneLinkDevice(int32_t key)
{
    this->key = key;
    this->lock = new std::mutex();
    sprintf(this->keyString, "%x", key);
}

int PhoneLinkDevice::operator==(PhoneLinkDevice device)
{
    if (this->key == device.key)
    {
        return true;
    }
    return false;
}

DeviceUnit *PhoneLinkDevice::lookUpDevice(DeviceUnit *unit)
{
    for (std::list<DeviceUnit *>::iterator i = this->deviceUnitList.begin(); i != this->deviceUnitList.end(); i++)
    {
        DeviceUnit *device = (*i);
        if ((*device == *unit) == true)
        {
            errorOut("%s",unit->name);
            return device;
        }
    }
    return NULL;
}

DeviceUnit *PhoneLinkDevice::addDevice(DeviceUnit *unit)
{
    this->lock->lock();
    //将unit添加到list中
    this->deviceUnitList.push_back(unit);
    std::list<DeviceUnit *>::iterator it = --(this->deviceUnitList.end());
    this->lock->unlock();
    return *it;
}

int PhoneLinkDevice::deleteDevice(DeviceUnit *unit)
{
    this->lock->lock();
    for (std::list<DeviceUnit *>::iterator i = this->deviceUnitList.begin(); i != this->deviceUnitList.end();)
    {
        if (*i == unit)
        {
            i = this->deviceUnitList.erase(i);
        }
        else
        {
            i++;
        }
    }
    this->lock->unlock();
    unit = NULL;
    return 0;
}
