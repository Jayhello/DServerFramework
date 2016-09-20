#ifndef _USE_CELLNET_PACKET_SINGLE_EXENETSESSION_H
#define _USE_CELLNET_PACKET_SINGLE_EXENETSESSION_H

#include <memory>
#include "packet.h"
#include "NetSession.h"

/*  ֻ��дonMsg--ʹ��cellnet packetЭ����Ϊͨ�ŵĻ�������Ự  */

class UseCellnetPacketSingleNetSession : public BaseNetSession, public std::enable_shared_from_this<UseCellnetPacketSingleNetSession>
{
public:
    UseCellnetPacketSingleNetSession();

private:
    virtual size_t  onMsg(const char* buffer, size_t len) final;
    virtual void    procPacket(uint32_t op, const char* body, PACKET_LEN_TYPE bodyLen) = 0;

private:
    uint16_t        mRecvSerialID;
};

#endif