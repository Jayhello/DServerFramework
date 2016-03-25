#ifndef _ETCD_CLIENT_H
#define _ETCD_CLIENT_H

#include "HttpServer.h"
#include "HttpParser.h"
#include "HttpFormat.h"

static HTTPParser etcdHelp(const std::string& ip, int port, HttpFormat::HTTP_TYPE_PROTOCOL protocol, const std::string& url,
    const std::map<std::string, std::string>& kv, int timeout)
{
    HTTPParser result(HTTP_BOTH);

    std::mutex mtx;
    std::condition_variable cv;

    HttpServer server;
    Timer::WeakPtr timer;
    server.startWorkThread(1);  /*����Ϊ1���߳�*/

    sock fd = ox_socket_connect(ip.c_str(), port);
    if (fd != SOCKET_ERROR)
    {
        server.addConnection(fd, [kv, url, &mtx, &cv, &server, &timer, timeout, protocol](HttpSession::PTR session){
            /*ע�ᳬʱ��ʱ��*/
            timer = server.getServer()->getService()->getRandomEventLoop()->getTimerMgr().AddTimer(timeout, [session](){
                session->getSession()->postClose();
            });

            HttpFormat request;
            request.addHeadValue("Accept", "*/*");
            request.setProtocol(protocol);
            std::string keyUrl = "/v2/keys/";
            keyUrl.append(url);
            request.setRequestUrl(keyUrl.c_str());
            if (!kv.empty())
            {
                for (auto& v : kv)
                {
                    request.addParameter(v.first.c_str(), v.second.c_str());
                }
                request.setContentType("application/x-www-form-urlencoded");
            }
            string requestStr = request.getResult();
            session->getSession()->send(requestStr.c_str(), requestStr.size());

        }, [&cv, &result, &timer](const HTTPParser& httpParser, HttpSession::PTR session, const char* websocketPacket, size_t websocketPacketLen){
            result = httpParser;
            /*�ر�����,��ɾ����ʱ��ʱ��*/
            session->getSession()->postClose();
            if (timer.lock() != nullptr)
            {
                timer.lock()->Cancel();
            }
        }, [&cv, &timer](HttpSession::PTR session){
            /*�յ��Ͽ�֪ͨ,֪ͨ�ȴ��߳�*/
            if (timer.lock() != nullptr)
            {
                timer.lock()->Cancel();
            }
            cv.notify_one();
        });

        cv.wait(std::unique_lock<std::mutex>(mtx));
    }

    return result;
}

static HTTPParser etcdSet(const std::string& ip, int port, const std::string& url, const std::map<std::string, std::string>& kv, int timeout)
{
    return etcdHelp(ip, port, HttpFormat::HTP_PUT, url, kv, timeout);
}

static HTTPParser etcdGet(const std::string& ip, int port, const std::string& url, int timeout)
{
    return etcdHelp(ip, port, HttpFormat::HTP_GET, url, {}, timeout);
}

#endif