#ifndef _CLIENT_LOGIC_OBJECT_H
#define _CLIENT_LOGIC_OBJECT_H

#include <unordered_map>
#include <stdint.h>
#include <memory>

#include "timer.h"
#include "NetSession.h"

class Packet;

/*  ���ӷ������ϵĿͻ��˶���(���ڽ�����������-֧�ֶ�������)(�˶���������ʾ��һ����ڷ��������߼���δ����) */
class ClientObject : public std::enable_shared_from_this<ClientObject>
{
public:
    typedef std::shared_ptr<ClientObject> PTR;
    typedef std::weak_ptr<ClientObject> WEAK_PTR;

    explicit ClientObject(int64_t id);

    void                resetSocketID(int64_t id);
    int64_t             getSocketID() const;

    ~ClientObject();

    /*֪ͨ�ڲ������������������Ͽ�*/
    void                notifyDisConnect();

    int64_t             getRuntimeID() const;

    /*  ��ҵ��ߣ�������ʱ�ȴ�   */
    void                startDelayWait();
    /*�Ƿ��ڶ��������ȴ�״̬*/
    bool                isInDelayWait();

    /*  ȡ���ȴ���ʱ�� */
    void                cancelDelayTimer();

    void                procPacket(PACKET_OP_TYPE op, const char* packerBuffer, PACKET_OP_TYPE packetLen);

    void                setSlaveServerID(int32_t id);

private:
    /*TODO::�Ƿ񱣳����ڵķ������Ựָ�룬����ÿ�β���*/
    void                sendPacketToPrimaryServer(Packet& packet);

    void                sendPacketToSlaveServer(Packet& packet);

    void                _sendPacketToServer(Packet& packet, unordered_map<int32_t, BaseNetSession::PTR>& servers, int32_t serverID);

    void                claimRuntimeID();
    void                claimPrimaryServer();

private:
    int64_t             mSocketID;
    int64_t             mRuntimeID;

    /*�ͻ����������غ󣬻��Զ�Ϊ�����primary �ͱ��logic server*/
    int                 mPrimaryServerID;       /*  �Զ�������ͻ��˵�logic server id    */

    /*���ڲ�logic server�������ÿͻ��˵�slave server��������ں󣬿ͻ��˷��͵�������Ϣ����ת������*/
    int                 mSlaveServerID;         /*  ��ʱ�ӹܿͻ��˵�logic server id */

    Timer::WeakPtr      mDelayTimer;
};

ClientObject::PTR findClientByRuntimeID(int64_t runtimeID);
void    addClientByRuntimeID(ClientObject::PTR client, int64_t runtimeID);
void    eraseClientByRuntimeID(int64_t runtimeID);

#endif