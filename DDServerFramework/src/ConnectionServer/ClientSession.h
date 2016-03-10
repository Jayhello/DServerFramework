#ifndef _CLIENT_SESSION_H
#define _CLIENT_SESSION_H

#include "NetSession.h"
#include "ClientLogicObject.h"
#include "UsePacketSingleNetSession.h"

/*  ������ӻỰ(���������ߣ��˶��������)    */
class ConnectionClientSession : public UsePacketSingleNetSession
{
public:
    typedef shared_ptr<ConnectionClientSession> PTR;

    ConnectionClientSession();
    ~ConnectionClientSession();

    ClientObject::PTR   getClientObject();

private:
    virtual void        onEnter() override;
    virtual void        onClose() override;
    virtual void        procPacket(PACKET_OP_TYPE op, const char* body, PACKET_LEN_TYPE bodyLen);

private:
    /*  �ͻ��������������(��������һ�����ӣ���������֮ǰ��ĳһ��ClientObject    */
    void                reConnect(const char* packerBuffer, PACKET_OP_TYPE packetLen);
private:
    ClientObject::PTR   mClient;    /*�߼���Ŀͻ��˶���*/
    int                 mPingCount;
};
#endif