#include "packet.h"
#include "WrapLog.h"

#include "ClientLogicObject.h"
#include "ClientSession.h"

extern WrapLog::PTR gDailyLogger;

ConnectionClientSession::ConnectionClientSession()
{
    gDailyLogger->info("ConnectionClientSession : {}, {}", getSocketID(), getIP());
    mClient = nullptr;
    mPingCount = 0;
}

ConnectionClientSession::~ConnectionClientSession()
{
    gDailyLogger->info("~ ConnectionClientSession : {}, {}", getSocketID(), getIP());
}

ClientObject::PTR ConnectionClientSession::getClientObject()
{
    return mClient;
}

void ConnectionClientSession::procPacket(PACKET_OP_TYPE op, const char* body, PACKET_LEN_TYPE bodyLen)
{
    mClient->procPacket(op, body - PACKET_HEAD_LEN, bodyLen + PACKET_HEAD_LEN);
}

void ConnectionClientSession::onEnter()
{
    gDailyLogger->warn("client enter, ip:{}, socket id :{}", getIP(), getSocketID());
    mClient = std::make_shared<ClientObject>(getSocketID());
}

void ConnectionClientSession::onClose()
{
    /*  TODO::�����������Ͽ�ʱ�����ڵ��߼��ͻ��˶��������������ĵȴ���ʱ��*/
    gDailyLogger->warn("client close, ip:{}, socket id :{}", getIP(), getSocketID());
    if (true)
    {
        gDailyLogger->warn("do not wait re connect, runtime id:{}", mClient->getRuntimeID());
        eraseClientByRuntimeID(mClient->getRuntimeID());
    }
    else
    {
        mClient->startDelayWait();
        mClient->notifyDisConnect();
    }
}

//  ���������ɿͻ��˷���user msg������Ȼ���پ���logic server������connection server������

///*  �ͻ��������������(��������һ�����ӣ���������֮ǰ��ĳһ��clientObject    */
//void ConnectionclientSession::reConnect(const char* packerBuffer, PACKET_OP_TYPE packetLen)
//{
//    /*������������ӻ�û�з���RuntimeID��Ҳ����û���ڲ������������κ���Դ*/
//    if (mclient->getRuntimeID() == -1)
//    {
//        /*TODO::������Ҫ��֤�Ϸ��ԣ���������������ĳһ���Ͽ��Ŀͻ��˶�����*/
//        ReadPacket rp(packerBuffer, packetLen);
//        rp.readPacketLen();
//        rp.readOP();
//        /*Ҫ�����Ŀͻ��˵�RuntimeID*/
//        int64_t oldRuntimeID = rp.readINT64();
//        ClientObject::PTR oldClient = findClientByRuntimeID(oldRuntimeID);
//        if (oldClient != nullptr && oldClient->isInDelayWait())
//        {
//            mClient = oldClient;
//            mClient->cancelDelayTimer();
//            mClient->resetSocketID(getSocketID());
//            mClient->notifyReConnect();
//        }
//    }
//}
