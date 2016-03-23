#include <string>

#include "../LogicServer/ClientMirror.h"
#include "../LogicServer/CenterServerConnection.h"

#include "packet.h"

#include "ClientExtOP.h"
#include "CenterServerExtRecvOP.h"

void initLogicServerExt()
{
    /*����ͻ��˷��͵���Ϣ*/
    ClientMirror::registerUserMsgHandle(CLIENT_OP_TEST, [](ClientMirror::PTR& client, ReadPacket& rp){
        client->sendv(CLIENT_OP_TEST, 1);
        if (false && gLogicCenterServerClient != nullptr)
        {
            std::string a = rp.readBinary();
            gLogicCenterServerClient->sendUserV(CENTER_SERVER_EXT_RECV_OP_TEST, "haha", 2);
            gLogicCenterServerClient->call("testrpc", "heihei", 3);
        }
    });

}