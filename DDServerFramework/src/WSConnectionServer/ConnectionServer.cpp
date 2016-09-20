#include <set>
#include <fstream>
#include <string>
#include <iostream>
#include <sstream>

using namespace std;

#include "WrapTCPService.h"
#include "timer.h"
#include "ox_file.h"
#include "WrapLog.h"
#include "AutoConnectionServer.h"
#include "etcdclient.h"
#include "WrapJsonValue.h"
#include "app_status.h"
#include "ClientSession.h"
#include "LogicServerSession.h"
#include "../../ServerConfig/ServerConfig.pb.h"
#include "google/protobuf/util/json_util.h"

WrapServer::PTR                         gServer;
ListenThread::PTR                       gListenClient;
ListenThread::PTR                       gListenLogic;
WrapLog::PTR                            gDailyLogger;

ServerConfig::ConnectionServerConfig connectionServerConfig;

/*
    ���ӷ��������ڷ������ܹ��д���N������
    1�������ַ����etcd��Ⱥ. (���ڵľ�����������Է���etcd����ȡ���е����ӷ�������ַ��������һ�����Ʒ�����ͻ���)��
    2��������ͻ��˺��ڲ��߼������������ӣ�ת���ͻ��˺��ڲ�������֮���ͨ��
*/

static void startServer()
{
    gServer = std::make_shared<WrapServer>();
    gServer->startWorkThread(ox_getcpunum(), [](EventLoop&){
    });

    /*��������ͻ��˶˿�*/
    gListenClient = std::make_shared<ListenThread>();
    gListenClient->startListen(connectionServerConfig.enableipv6(), connectionServerConfig.bindip(), connectionServerConfig.portforclient(), nullptr, nullptr, [&](sock fd){
        WrapAddNetSession(gServer, fd, std::make_shared<ConnectionClientSession>(), -1, 32 * 1024);
    });

    /*���������߼��������˿�*/
    gListenLogic = std::make_shared<ListenThread>();
    gListenLogic->startListen(connectionServerConfig.enableipv6(), connectionServerConfig.bindip(), connectionServerConfig.portforlogicserver(), nullptr, nullptr, [&](sock fd){
        WrapAddNetSession(gServer, fd, std::make_shared<LogicServerSession>(), 10000, 32 * 1024 * 1024);
    });
}

int main()
{
    ifstream  connetionServerConfigFile("ServerConfig/ConnetionServerConfig.json");
    std::stringstream buffer;
    buffer << connetionServerConfigFile.rdbuf();

    google::protobuf::util::Status s = google::protobuf::util::JsonStringToMessage(buffer.str(), &connectionServerConfig);
    if (!s.ok())
    {
        std::cerr << "load config error:" << s.error_message() << std::endl;
        exit(-1);
    }

    ox_dir_create("logs");
    ox_dir_create("logs/ConnectionServer");

    gDailyLogger = std::make_shared<WrapLog>();
    gDailyLogger->setFile("", "logs/ConnectionServer/daily");

    startServer();

    /*  ͬ��etcd  */
    std::map<string, string> etcdKV;

    etcdKV["value"] = buffer.str();
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

        for (auto& etcd : connectionServerConfig.etcdservers())
        {
            if (!etcdSet(etcd.ip(), etcd.port(), string("ConnectionServerList/") + std::to_string(connectionServerConfig.id()), etcdKV, 5000).getBody().empty())
            {
                break;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(5000));   /*5s ����һ��*/
    }

    gListenClient->closeListenThread();
    gListenLogic->closeListenThread();
    gServer->getService()->closeListenThread();
    gServer->getService()->stopWorkerThread();
    gDailyLogger->stop();

    return 0;
}