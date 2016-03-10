#include <string>
#include "SocketLibTypes.h"
#include "WrapLog.h"
#include "../CenterServer/CenterServerSession.h"
#include "CenterServerExtRecvOP.h"

extern WrapLog::PTR gDailyLogger;

void initCenterServerExt()
{
    /*���������ڲ���������rpc����*/
    /*TODO::�󶨷��ʹ�rpc��center session*/
    gCenterServerSessionRpc.def("testrpc", [](const std::string& a, int b){
        gDailyLogger->info("rpc handle: {} - {}", a, b);
    });

    /*���������ڲ����������͵�OP��Ϣ*/
    CenterServerSession::registerUserMsgHandle(CENTER_SERVER_EXT_RECV_OP_TEST, [](CenterServerSession&, ReadPacket& rp){
        std::string a = rp.readBinary();
        int b = rp.readINT32();
    });
}