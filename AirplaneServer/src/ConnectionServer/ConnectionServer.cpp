#include <set>

#include "WrapTCPService.h"
#include "NetSession.h"
#include "socketlibfunction.h"
#include "platform.h"
#include "packet.h"
#include "timer.h"
#include "ox_file.h"
#include "WrapLog.h"
#include "ClientSession.h"
#include "LogicServerSession.h"
#include "lua_readtable.h"
#include "HelpFunction.h"
#include "ConnectionServerPassword.h"
#include "AutoConnectionServer.h"
#include "etcdclient.h"
#include "WrapJsonValue.h"
#include "app_status.h"

WrapLog::PTR gDailyLogger;

/*  ���е�primary server��slave server����(keyΪ�ڲ���Ϸ����������ʱ��������߼�ID   */
unordered_map<int, BaseNetSession::PTR>     gAllPrimaryServers;
unordered_map<int, BaseNetSession::PTR>     gAllSlaveServers;

WrapServer::PTR                         gServer;
TimerMgr::PTR                           gTimerMgr;

string selfIP;
int gSelfID = 0;
int portForClient;
int portForLogicServer;

/*
    ���ӷ��������ڷ������ܹ��д���N������
    1�������ַ����etcd��Ⱥ. (���ڵľ�����������Է���etcd����ȡ���е����ӷ�������ַ��������һ�����Ʒ�������)��
    2����������Һ��ڲ��߼������������ӣ�ת����Һ��ڲ�������֮���ͨ��
*/

int main()
{
    std::vector<std::tuple<string, int>> etcdServers;

    try
    {
        struct msvalue_s config(true);
        struct lua_State* L = luaL_newstate();
        luaopen_base(L);
        luaL_openlibs(L);
        /*TODO::����������ָ������·��*/
        ConnectionServerPassword::getInstance().load(L);
        if (lua_tinker::dofile(L, "ServerConfig//ConnectionServerConfig.lua"))
        {
            aux_readluatable_byname(L, "ConnectionServerConfig", &config);
        }
        else
        {
            throw std::runtime_error("not found ServerConfig//ConnectionServerConfig.lua file");
        }

        map<string, msvalue_s*>& _submapvalue = *config._map;

        selfIP = map_at(_submapvalue, string("selfIP"))->_str;
        portForClient = atoi(map_at(_submapvalue, string("portForClient"))->_str.c_str());
        portForLogicServer = atoi(map_at(_submapvalue, string("portForLogicServer"))->_str.c_str());
        gSelfID = atoi(map_at(_submapvalue, string("id"))->_str.c_str());
        map<string, msvalue_s*>& etcdConfig = *map_at(_submapvalue, string("etcdservers"))->_map;
        for (auto& v : etcdConfig)
        {
            map<string, msvalue_s*>& oneconfig = *((v.second)->_map);
            etcdServers.push_back(std::make_tuple(map_at(oneconfig, string("ip"))->_str, atoi(map_at(oneconfig, string("port"))->_str.c_str())));
        }
        lua_close(L);
        L = nullptr;
    }
    catch (const std::exception& e)
    {
        errorExit(e.what());
    }

    gDailyLogger = std::make_shared<WrapLog>();
    gTimerMgr = std::make_shared<TimerMgr>();

    gServer = std::make_shared<WrapServer>();
    /*  �����߳���Ϊ1�� gTimerMgr ��ȫ���һ���ȫ���߼����ӹ���1���߳��ǰ�ȫ��*/
    gServer->startWorkThread(1, [](EventLoop&){
        gTimerMgr->Schedule();
    });

    ox_dir_create("logs");
    ox_dir_create("logs/ConnectionServer");
    gDailyLogger->setFile("", "logs/ConnectionServer/daily");

    gDailyLogger->info("start port:{} for client", portForClient);
    /*��������ͻ��˶˿�*/
    ListenThread    clientListen;
    clientListen.startListen(portForClient, nullptr, nullptr, [&](int fd){
        WrapAddNetSession(gServer, fd, std::make_shared<ConnectionClientSession>(), -1, 32*1024);
    });

    gDailyLogger->info("start port:{} for logic server", portForLogicServer);
    /*���������߼��������˿�*/
    ListenThread logicServerListen;
    logicServerListen.startListen(portForLogicServer, nullptr, nullptr, [&](int fd){
        WrapAddNetSession(gServer, fd, std::make_shared<LogicServerSession>(), 10000, 32*1024*1024);
    });

    std::map<string, string> etcdKV;

    WrapJsonValue serverJson(rapidjson::kObjectType);
    serverJson.AddMember("ID", gSelfID);
    serverJson.AddMember("IP", selfIP);
    serverJson.AddMember("portForClient", portForClient);
    serverJson.AddMember("portForLogicServer", portForLogicServer);

    etcdKV["value"] = serverJson.toString();
    etcdKV["ttl"] = std::to_string(15); /*���ʱ��Ϊ15��*/

    while (true)
    {
        if (app_kbhit())
        {
            string input;
            std::getline(std::cin, input);
            gDailyLogger->warn("console input {}", input);

            if (input == "quit")
            {
                break;
            }
        }

        for (auto& etcd : etcdServers)
        {
            if (!etcdSet(std::get<0>(etcd), std::get<1>(etcd), string("ConnectionServerList/") + std::to_string(gSelfID), etcdKV, 5000).getBody().empty())
            {
                break;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(5000));   /*5s ����һ��*/
    }

    return 0;
}