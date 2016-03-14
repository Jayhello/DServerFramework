#ifndef _LOGIC_CLIENT_MIRROR_H
#define _LOGIC_CLIENT_MIRROR_H

#include <stdint.h>
#include <unordered_map>
#include <memory>
#include <functional>

#include "packet.h"

class ReadPacket;
class Packet;
class WrapJsonValue;
class ConnectionServerConnection;

/*  logic server�ϵĿͻ��˾��� */
class ClientMirror
{
public:
    typedef std::shared_ptr<ClientMirror>   PTR;
    typedef std::function<void(ClientMirror::PTR)> ENTER_HANDLE;
    typedef std::function<void(ClientMirror::PTR)> DISCONNECT_HANDLE;

    ClientMirror(int32_t csID, int64_t runtimeID);
    ~ClientMirror();

    typedef std::function<void(ClientMirror&, ReadPacket&)>   USER_MSG_HANDLE;

    /*�������ÿͻ��������ӷ������ϵ�session id (�ɶ��������ɹ���)*/
    void            setSocketIDOnConnectionServer(int64_t socketID);

    int32_t         getConnectionServerID() const;
    int64_t         getRuntimeID() const;

    void            sendPacket(Packet& packet);
    void            sendPacket(const std::string& realPacketBinary);
    void            sendPacket(const char* buffer, size_t len);

    template<typename... Args>
    void            sendv(PACKET_OP_TYPE op, const Args&... args)
    {
        BigPacket packet(op);
        packet.writev(args...);
        sendPacket(packet);
    }

    /*  ����ͻ����������ӷ���������(��ȡ������)��ǰ�ͻ��˵�slaveΪ��ǰlogic server*/
    void            requestConnectionServerSlave(bool isSet);
public:
    static void     registerUserMsgHandle(PACKET_OP_TYPE, USER_MSG_HANDLE);

    static void     setClientEnterCallback(ENTER_HANDLE);
    static void     setClientDisConnectCallback(DISCONNECT_HANDLE);

    static  ENTER_HANDLE        getClientEnterCallback();
    static  DISCONNECT_HANDLE   getClientDisConnectCallback();
private:
    void            procData(const char* buffer, size_t len);

private:
    const int32_t   mConnectionServerID;            /*  �˿ͻ����������ӷ�������ID   */
    const int64_t   mRuntimeID;                     /*  �˿ͻ�������Ϸ����ʱ��ID    */
    int64_t         mSocketIDOnConnectionServer;    /*  �˿ͻ������������ӷ������ϵ�socketid   */

private:
    static  std::unordered_map<PACKET_OP_TYPE, USER_MSG_HANDLE> sUserMsgHandlers;
    static  ENTER_HANDLE        sEnterHandle;                   /*  �ͻ��˵Ľ���ص�    */
    static  DISCONNECT_HANDLE   sDisConnectHandle;          /*  �ͻ��˵ĶϿ��ص�*/

    friend class ConnectionServerConnection;
};


class ClientMirrorMgr
{
public:
    typedef std::shared_ptr<ClientMirrorMgr> PTR;
    /*key Ϊ�ͻ��˵�RuntimeID*/
    typedef std::unordered_map<int64_t, ClientMirror::PTR> CLIENT_MIRROR_MAP;
public:
    ClientMirror::PTR                           FindClientByRuntimeID(int64_t id);
    void                                        AddClientOnRuntimeID(ClientMirror::PTR p, int64_t id);
    void                                        DelClientByRuntimeID(int64_t id);


    CLIENT_MIRROR_MAP&                          getAllClientMirror();
private:
    CLIENT_MIRROR_MAP                           mAllClientMirrorOnRuntimeID;
};

extern ClientMirrorMgr::PTR   gClientMirrorMgr;

#endif