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
    ClientMirror();
    ~ClientMirror();

    typedef std::function<void(ClientMirror&, ReadPacket&)>   USER_MSG_HANDLE;

    int32_t         getConnectionServerID() const;

    void            setRuntimeInfo(int32_t csID, int64_t socketID, int64_t runtimeID);
    int64_t         getRuntimeID() const;

    void            sendPacket(Packet& packet);
    void            sendPacket(const std::string& realPacketBinary);
    void            sendPacket(const char* buffer, size_t len);

    /*����������������ӷ������ϵ�session id (���������ɹ���)*/
    void            resetSocketInfo(int64_t socketID);

    template<typename... Args>
    void            sendv(int16_t op, const Args&... args)
    {
        BigPacket packet(op);
        packet.writev(args...);
        sendPacket(packet);
    }

    void            requestConnectionServerSlave(bool isSet);
public:
    static void     registerUserMsgHandle(PACKET_OP_TYPE, USER_MSG_HANDLE);

private:
    void            procData(const char* buffer, size_t len);

private:
    int32_t         mConnectionServerID;            /*  ������������ӷ�������ID   */
    int64_t         mSocketIDOnConnectionServer;    /*  ��������������ӷ������ϵ�socketid   */
    int64_t         mRuntimeID;                     /*  ���������Ϸ����ʱ��ID    */

private:
    static  std::unordered_map<PACKET_OP_TYPE, USER_MSG_HANDLE> sUserMsgHandlers;

    friend class ConnectionServerConnection;
};


class ClientMirrorMgr
{
public:
    typedef std::shared_ptr<ClientMirrorMgr> PTR;
    /*key Ϊ�ͻ��˵�RuntimeID*/
    typedef std::unordered_map<int64_t, ClientMirror*> CLIENT_MIRROR_MAP;
public:
    ClientMirror*                               FindClientByRuntimeID(int64_t id);
    void                                        AddClientOnRuntimeID(ClientMirror* p, int64_t id);
    void                                        DelClientByRuntimeID(int64_t id);


    CLIENT_MIRROR_MAP&                          getAllClientMirror();
private:
    CLIENT_MIRROR_MAP                           mAllClientMirrorOnRuntimeID;
};

extern ClientMirrorMgr::PTR   gClientMirrorMgr;

#endif