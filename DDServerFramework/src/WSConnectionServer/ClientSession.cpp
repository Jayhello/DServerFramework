#include <unordered_map>
#include <algorithm>

#include "packet.h"
#include "WrapLog.h"
#include "ClientSessionMgr.h"

#include "WebSocketFormat.h"
#include "LogicServerSessionMgr.h"

#include "../../ServerConfig/ServerConfig.pb.h"
#include "./proto/LogicServerWithConnectionServer.pb.h"

#include "ClientSession.h"

extern ServerConfig::ConnectionServerConfig     connectionServerConfig;
extern WrapLog::PTR                             gDailyLogger;
extern WrapServer::PTR                          gServer;
static std::atomic<int32_t> incRuntimeID = ATOMIC_VAR_INIT(0);

ConnectionClientSession::ConnectionClientSession()
{
    mRuntimeID = -1;
    mPrimaryServerID = -1;
    mSlaveServerID = -1;

    mRecvSerialID = 0;
    mSendSerialID = 0;

    mReconnecting = false;

    gDailyLogger->info("ConnectionClientSession : {}, {}", getSocketID(), getIP());
}

ConnectionClientSession::~ConnectionClientSession()
{
    gDailyLogger->info("~ ConnectionClientSession : {}, {}", getSocketID(), getIP());
}

union CLIENT_RUNTIME_ID
{
    int64_t id;
    struct
    {
        int16_t serverID;
        int16_t incID;
        int32_t time;
    }humman;
};

void ConnectionClientSession::claimRuntimeID()
{
    if (mRuntimeID == -1)
    {
        static_assert(sizeof(union CLIENT_RUNTIME_ID) == sizeof(((CLIENT_RUNTIME_ID*)nullptr)->id), "");

        CLIENT_RUNTIME_ID tmp;
        tmp.humman.serverID = connectionServerConfig.id();
        tmp.humman.incID = std::atomic_fetch_add(&incRuntimeID, 1);
        tmp.humman.time = static_cast<int32_t>(time(nullptr));

        mRuntimeID = tmp.id;
        ClientSessionMgr::AddClientByRuntimeID(std::static_pointer_cast<ConnectionClientSession>(shared_from_this()), mRuntimeID);
    }
}

void ConnectionClientSession::claimPrimaryServer()
{
    assert(mRuntimeID != -1);
    if (mRuntimeID != -1 && mPrimaryServer == nullptr)
    {
        mPrimaryServerID = LogicServerSessionMgr::ClaimPrimaryLogicServer();
        if (mPrimaryServerID != -1)
        {
            mPrimaryServer = LogicServerSessionMgr::FindPrimaryLogicServer(mPrimaryServerID);
        }
        else
        {
            gDailyLogger->error("claim primary server failed of runtime id :{}, ip:{}", mRuntimeID, getIP());
        }
    }
}

void ConnectionClientSession::setPrimaryServerID(int id)
{
    mPrimaryServerID = id;
}

int ConnectionClientSession::getPrimaryServerID() const
{
    return mPrimaryServerID;
}

int64_t ConnectionClientSession::getRuntimeID() const
{
    return mRuntimeID;
}

void ConnectionClientSession::notifyServerPlayerExist()
{
    gDailyLogger->info("ready notify client {} exist game", mRuntimeID);

    internalAgreement::CloseClientACK closeAck;
    closeAck.set_clientid(mRuntimeID);

    const int CLOSE_CLIENT_ACK_OP = 1513370829;

    // ���û���ҵ���������������,���������������,��ô��Ҫ�����з��������ͶϿ�(�����̨�����ɹ�)
    //�����з�����֪ͨ����(����ĳ���������ɹ���;��-�߼���������û�з��������ӷ�����,Ȼ��������������϶Ͽ�,Ȼ���Ժ�����˾���Ҳ�ҵ������ӣ������޷��ߵ���������Ӧ���߼�����)

    auto server = mPrimaryServer;
    if (server == nullptr && mPrimaryServerID != -1)
    {
        server = LogicServerSessionMgr::FindPrimaryLogicServer(mPrimaryServerID);
    }
    if (server != nullptr)
    {
        gDailyLogger->info("really notify client {} exist primary server", mRuntimeID);
        server->sendPB(CLOSE_CLIENT_ACK_OP, closeAck);
    }
    else if (mReconnecting)
    {
        auto PrimaryServers = LogicServerSessionMgr::GetAllPrimaryLogicServer();
        for (auto& v : PrimaryServers)
        {
            v.second->sendPB(CLOSE_CLIENT_ACK_OP, closeAck);
        }
    }

    server = mSlaveServer;
    if (server == nullptr && mSlaveServerID != -1)
    {
        server = LogicServerSessionMgr::FindSlaveLogicServer(mSlaveServerID);
    }
    if (server != nullptr)
    {
        gDailyLogger->info("really notify client {} exist salve server", mRuntimeID);
        server->sendPB(CLOSE_CLIENT_ACK_OP, closeAck);
    }
    else if (mReconnecting)
    {
        auto SlaveServers = LogicServerSessionMgr::GetAllSlaveLogicServer();
        for (auto& v : SlaveServers)
        {
            v.second->sendPB(CLOSE_CLIENT_ACK_OP, closeAck);
        }
    }
}

void ConnectionClientSession::sendPBBinary(int32_t cmd, const char* data, size_t len)
{
    if (getEventLoop()->isInLoopThread())
    {
        helpSendPacket(cmd, data, len);
    }
    else
    {
        sendPBBinary(cmd, std::make_shared<std::string>(data, len));
    }
}

void ConnectionClientSession::sendPBBinary(int32_t cmd, std::shared_ptr<std::string>&& data)
{
    sendPBBinary(cmd, data);
}

void ConnectionClientSession::sendPBBinary(int32_t cmd, std::shared_ptr<std::string>& data)
{
    if (getEventLoop()->isInLoopThread())
    {
        helpSendPacket(cmd, data->c_str(), data->size());
    }
    else
    {
        getEventLoop()->pushAsyncProc([=](){
            helpSendPacket(cmd, data->c_str(), data->size());
        });
    }
}

void ConnectionClientSession::helpSendPacket(uint32_t op, const char* data, size_t len)
{
    assert(getEventLoop()->isInLoopThread());

    char b[8 * 1024];
    BasePacketWriter packet(b, sizeof(b), true);
    packet.writeINT8('{');
    packet.writeUINT32(static_cast<uint32_t>(len + 14));
    packet.writeUINT32(op);
    packet.writeUINT32(mSendSerialID++);
    packet.writeBuffer(data, len);
    packet.writeINT8('}');

    auto frame = std::make_shared<std::string>();
    if (WebSocketFormat::wsFrameBuild(packet.getData(), packet.getPos(), *frame, WebSocketFormat::WebSocketFrameType::TEXT_FRAME))
    {
        sendPacket(frame);
    }
}

void ConnectionClientSession::setSlaveServerID(int id)
{
    gDailyLogger->info("set client {} slave server id:{}", mRuntimeID, id);
    mSlaveServerID = id;
}

void ConnectionClientSession::procPacket(uint32_t op, const char* body, uint32_t bodyLen)
{
    const static int UPSTREAM_ACK_OP = 2210821449;
    const static std::vector<uint32_t> PRIMARY_OPS = { 24, 52, 74, 75 };

    mRecvSerialID++;

    if (op == 10000)
    {
        sendPBBinary(10000, "", 0);
        return;
    }

    gDailyLogger->info("recv op {} from id:{}", op, mRuntimeID);

    internalAgreement::UpstreamACK upstream;
    upstream.set_clientid(mRuntimeID);
    upstream.set_msgid(op);
    upstream.set_data(body, bodyLen);

    if (op == 91)  
    {
        /*  ����������Ϊ��һ����Ϣ��    */
        if (mRecvSerialID == 1)
        {
            auto allPrimaryServer = LogicServerSessionMgr::GetAllPrimaryLogicServer();
            for (auto& server : allPrimaryServer)
            {
                server.second->sendPB(UPSTREAM_ACK_OP, upstream);
            }
        }

        mReconnecting = true;

        return;
    }

    if (mPrimaryServerID == -1)
    {
        /*  ֻ�д�����û��������������������������������� */
        if (!mReconnecting)
        {
            claimPrimaryServer();
        }
    }
    else
    {
        if (mPrimaryServer == nullptr)
        {
            assert(mReconnecting);
            /*  �����������ID��������Ч,�Ҳ�û�г�ʼ������������������,��Ϊ�����ɹ�����,�ڴ˳�ʼ�������������������� */
            mPrimaryServer = LogicServerSessionMgr::FindPrimaryLogicServer(mPrimaryServerID);
            mReconnecting = false;
        }
    }

    if (mSlaveServerID == -1)
    {
        mSlaveServer = nullptr;
    }
    else
    {
        if (mSlaveServer == nullptr)
        {
            mSlaveServer = LogicServerSessionMgr::FindSlaveLogicServer(mSlaveServerID);
        }
    }

    if (std::find(PRIMARY_OPS.begin(), PRIMARY_OPS.end(), op) != PRIMARY_OPS.end())   /*  �ض���Ϣ�̶�ת��������������߼�������(TODO::������OPö�ٶ���) */
    {
        if (mPrimaryServer != nullptr)
        {
            mPrimaryServer->sendPB(UPSTREAM_ACK_OP, upstream);
        }
        else
        {
            gDailyLogger->error("primary server is nil");
        }
    }
    else
    {
        if (mSlaveServer != nullptr)
        {
            mSlaveServer->sendPB(UPSTREAM_ACK_OP, upstream);
        }
        else if (mPrimaryServer != nullptr)
        {
            mPrimaryServer->sendPB(UPSTREAM_ACK_OP, upstream);
        }
        else
        {
            gDailyLogger->error("no any server process this packet cmd:{} of client {}, ip is {}", op, mRuntimeID, getIP());
        }
    }
}

void ConnectionClientSession::onEnter()
{
    claimRuntimeID();
    gDailyLogger->info("client enter, ip:{}, socket id :{}, runtime id:{}", getIP(), getSocketID(), mRuntimeID);
}

void ConnectionClientSession::onClose()
{
    gDailyLogger->info("client close, ip:{}, socket id :{}, runtime id:{}", getIP(), getSocketID(), getRuntimeID());
    if (mRuntimeID != -1)
    {
        auto sharedThis = std::static_pointer_cast<ConnectionClientSession>(shared_from_this());
        getEventLoop()->getTimerMgr()->addTimer(2 * 60 * 1000, [=](){
            sharedThis->notifyServerPlayerExist();
            ClientSessionMgr::EraseClientByRuntimeID(sharedThis->getRuntimeID());
            //����onClose�����EraseClientByRuntimeID������Ϊ�п�������ٴε�¼ʱ���ȴ�������Ͽ��ٴ���kick����ô����Ͽ����Ƴ�����ң����ҵȴ�2������֪ͨ�˳���Ҳ���߶���������
            //��ôkick�߼�����޷�����֪ͨ�������(������޷���ʱ�����ٴε�¼)(��Ϊ�Ҳ�����Ҷ�����)
        });
    }
}