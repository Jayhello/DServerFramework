#ifndef _NETSESSION_H
#define _NETSESSION_H

#include <string>
#include "WrapTCPService.h"

/*Ӧ�÷������������Ự�������*/
class BaseNetSession
{
public:
    typedef std::shared_ptr<BaseNetSession>  PTR;

    BaseNetSession()
    {
        mServer = nullptr;
    }

    virtual ~BaseNetSession()
    {
        mSocketID = 0;
    }

    void    setSession(WrapServer::PTR server, int64_t socketID, const std::string& ip)
    {
        mServer = server;
        mSocketID = socketID;
        mIP = ip;
    }

    WrapServer::PTR getServer()
    {
        return mServer;
    }

    /*�����յ�������*/
    virtual size_t  onMsg(const char* buffer, size_t len) = 0;
    /*���ӽ���*/
    virtual void    onEnter() = 0;
    /*���ӶϿ�*/
    virtual void    onClose() = 0;

    const std::string&  getIP() const
    {
        return mIP;
    }

    int64_t         getSocketID() const
    {
        return mSocketID;
    }

    void            postClose()
    {
        mServer->getService()->disConnect(mSocketID);
    }

    void            sendPacket(const char* data, int len, const DataSocket::PACKED_SENDED_CALLBACK& callback = nullptr)
    {
        mServer->getService()->send(mSocketID, DataSocket::makePacket(data, len), callback);
    }

    void            sendPacket(const DataSocket::PACKET_PTR& packet, const DataSocket::PACKED_SENDED_CALLBACK& callback = nullptr)
    {
        mServer->getService()->send(mSocketID, packet, callback);
    }

    EventLoop*      getEventLoop()
    {
        return mServer->getService()->getEventLoopBySocketID(mSocketID);
    }
private:
    std::string         mIP;
    WrapServer::PTR     mServer;
    int64_t             mSocketID;
};

void WrapAddNetSession(WrapServer::PTR server, int fd, BaseNetSession::PTR pClient, int pingCheckTime, int maxRecvBufferSize);

#endif