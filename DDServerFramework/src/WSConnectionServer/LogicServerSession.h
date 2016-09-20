#ifndef _LOGIC_SERVER_SESSION_H
#define _LOGIC_SERVER_SESSION_H

#include <memory>
#include <string>
#include "NetSession.h"
#include "UseCellnetSingleNetSession.h"

class ReadPacket;

/*  �߼����������� */
class LogicServerSession : public UseCellnetPacketSingleNetSession
{
public:
    typedef std::shared_ptr<LogicServerSession> PTR;

    LogicServerSession();

    void                sendPBData(uint32_t cmd, const char* data, size_t len);

    template<typename T>
    void                sendPB(uint32_t cmd, const T& t)
    {
        char buff[8 * 1024];
        if (t.SerializeToArray((void*)buff, t.ByteSize()))
        {
            sendPBData(cmd, buff, t.ByteSize());
        }
    }

private:
    virtual     void    onEnter() override;
    virtual     void    onClose() override;

    void                procPacket(uint32_t op, const char* body, PACKET_LEN_TYPE bodyLen);

private:
    /*  �ڲ���������½�����ӷ�����   */
    void                onLogicServerLogin(ReadPacket& rp);
    /*  �ڲ�����������ת����Ϣ���ͻ���(��RuntimeID��ʶ) */
    void                onPacket2ClientByRuntimeID(ReadPacket& rp);

    void                onSlaveServerIsSetClient(ReadPacket& rp);
    /*  ǿ���߳�ĳRuntimeID����ʶ�Ŀͻ���   */
    void                onKickClientByRuntimeID(ReadPacket& rp);

private:
    bool                checkPassword(const std::string& password);
    void                sendLogicServerLoginResult(bool isSuccess, const std::string& reason);

private:
    bool                mIsPrimary;
    int                 mID;

    uint16_t            mSendSerialID;
};

#endif