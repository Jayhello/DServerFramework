#ifndef _LOGIC_SERVER_SESSION_H
#define _LOGIC_SERVER_SESSION_H

#include "NetSession.h"
#include "UsePacketSingleNetSession.h"

class ReadPacket;

/*�߼�����������*/
class LogicServerSession : public UsePacketSingleNetSession
{
public:
    LogicServerSession();

private:
    virtual     void    onEnter() override;
    virtual     void    onClose() override;

    void                procPacket(PACKET_OP_TYPE op, const char* body, PACKET_LEN_TYPE bodyLen);

private:
    /*�ڲ���������½�����ӷ�����*/
    void                onLogicServerLogin(ReadPacket& rp);
    /*�ڲ�����������ת����Ϣ���ͻ���(��SocketID��ʶ)*/
    void                onPacket2ClientBySocketInfo(ReadPacket& rp);
    /*�ڲ�����������ת����Ϣ���ͻ���(��RuntimeID��ʶ)*/
    void                onPacket2ClientByRuntimeID(ReadPacket& rp);

    void                onSlaveServerIsSetClient(ReadPacket& rp);
    /*ǿ���߳�ĳRuntimeID����ʶ�Ŀͻ���*/
    void                onKickClientByRuntimeID(ReadPacket& rp);

    void                onPing(ReadPacket& rp);

private:
    bool                checkPassword(const string& password);
    void                sendLogicServerLoginResult(bool isSuccess, const string& reason);

private:
    bool                mIsPrimary;
    int                 mID;
};


#endif