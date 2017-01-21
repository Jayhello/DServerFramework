#ifndef _LOGIC_SERVER_SESSION_H
#define _LOGIC_SERVER_SESSION_H

#include <memory>
#include <string>
#include <atomic>

#include "NetSession.h"
#include "UseCellnetSingleNetSession.h"

class BasePacketReader;

/*  �߼����������� */
class LogicServerSession : public UseCellnetPacketSingleNetSession
{
public:
    typedef std::shared_ptr<LogicServerSession> PTR;

    LogicServerSession();

    int                 getID() const;

    template<typename T>
    void                sendPB(UseCellnetPacketSingleNetSession::CELLNET_OP_TYPE cmd, const T& t)
    {
        if (getEventLoop()->isInLoopThread())
        {
            char buff[8 * 1024];
            if (t.SerializePartialToArray((void*)buff, sizeof(buff)))
            {
                sendPBData(cmd, buff, t.ByteSize());
            }
        }
        else
        {
            auto str = t.SerializePartialAsString();
            if (!str.empty())
            {
                auto smartStr = std::make_shared<std::string>();
                *smartStr = std::move(str);
                sendPBData(cmd, smartStr);
            }
        }
    }

private:
    virtual     void    onEnter() override;
    virtual     void    onClose() override;

    void                procPacket(UseCellnetPacketSingleNetSession::CELLNET_OP_TYPE op, const char* body, uint16_t bodyLen) override;

private:
    /*  �ڲ���������½�����ӷ�����   */
    void                onLogicServerLogin(BasePacketReader& rp);
    /*  �ڲ�����������ת����Ϣ���ͻ���(��RuntimeID��ʶ) */
    void                onPacket2ClientByRuntimeID(BasePacketReader& rp);

    void                onIsSetPlayerSlaveServer(BasePacketReader& rp);
    void                onSetPlayerPrimaryServer(BasePacketReader& rp);
    /*  ǿ���߳�ĳRuntimeID����ʶ�Ŀͻ���   */
    void                onKickClientByRuntimeID(BasePacketReader& rp);

private:
    bool                checkPassword(const std::string& password);
    void                sendLogicServerLoginResult(bool isSuccess, const std::string& reason);

    void                sendPBData(UseCellnetPacketSingleNetSession::CELLNET_OP_TYPE cmd, const char* data, size_t len);
    void                sendPBData(UseCellnetPacketSingleNetSession::CELLNET_OP_TYPE cmd, std::shared_ptr<std::string>&);
    void                sendPBData(UseCellnetPacketSingleNetSession::CELLNET_OP_TYPE cmd, std::shared_ptr<std::string>&&);

    void                helpSendPacketInLoop(UseCellnetPacketSingleNetSession::CELLNET_OP_TYPE cmd, const char* data, size_t len);
private:
    bool                mIsPrimary;
    int                 mID;

    std::atomic<uint16_t>    mSendSerialID;
};

#endif