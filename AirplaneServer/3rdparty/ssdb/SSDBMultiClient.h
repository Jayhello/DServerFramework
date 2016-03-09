#ifndef _SSDB_PROXY_CLIENT_H
#define _SSDB_PROXY_CLIENT_H

#include <string>
#include <functional>
#include <unordered_map>
#include <stdint.h>
#include <vector>
#include <thread>
#include <memory>

#include "eventloop.h"
#include "datasocket.h"
#include "msgqueue.h"
#include "SSDBProtocol.h"

using namespace std;

class DataSocket;
class SSDBProtocolRequest;
class RedisProtocolRequest;
struct parse_tree;

/*  �����Ӷ��ssdb �������Ŀͻ���   */

/*  TODO::Ŀǰ����ssdb���������ѡ�����������������Ҳ��Ҫ�ṩ������ʹ����ȥ�������ӣ��Լ��ͻ����Զ���sharding    */

/*  TODO::Ŀǰ��Ҫ֧��ssdb����redisֻ֧��get/set����redis��ssdb��ת��Ҳû�в���   */

class SSDBMultiClient
{
public:
    typedef SSDBProtocolRequest MyRequestProcotol;

    typedef std::shared_ptr<SSDBMultiClient> PTR;

    typedef std::function<void(const std::string&, const Status&)>  ONE_STRING_CALLBACK;
    typedef std::function<void(const Status&)>                      NONE_VALUE_CALLBACK;
    typedef std::function<void(int64_t, const Status&)>             ONE_INT64_CALLBACK;
    typedef std::function<void(const std::vector<std::string>&, const Status&)> STRING_LIST_CALLBACK;

public:
    SSDBMultiClient();
    ~SSDBMultiClient();

    EventLoop&                                                      getEventLoop();

    void                                                            startNetThread(std::function<void(void)> frameCallback = nullptr);
    /*����һ���첽���Ӻ�˷�����, pingTimeΪpingʱ�䣬-1��ʾ��ִ��ping����, isAutoConnectionΪ�Ƿ��Զ�����*/
    void                                                            asyncConnectionProxy(string ip, int port, int pingTime, bool isAutoConnection);
    /*���һ��(�����ӵ�)��˷�����(�̰߳�ȫ)*/
    void                                                            addConnectionProxy(sock fd, string ip, int port, int pingTime, bool isAutoConnection);

    void                                                            redisSet(const std::string& key, const std::string& value, const NONE_VALUE_CALLBACK& callback);
    void                                                            redisGet(const std::string& key, const ONE_STRING_CALLBACK& callback);

    void                                                            multiSet(const std::unordered_map<std::string, std::string> &kvs, const NONE_VALUE_CALLBACK& callback);
    void                                                            multiGet(const std::vector<std::string> &keys, const STRING_LIST_CALLBACK& callback);
    void                                                            multiDel(const std::vector<std::string> &keys, const NONE_VALUE_CALLBACK& callback);

    void                                                            getset(const std::string& key, const std::string& value, const ONE_STRING_CALLBACK& callback);

    void                                                            set(const std::string& key, const std::string& value, const NONE_VALUE_CALLBACK& callback);
    void                                                            get(const string& k, const ONE_STRING_CALLBACK& callback);

    void                                                            hget(const string& hname, const string& k, const ONE_STRING_CALLBACK& callback);
    void                                                            hset(const string& hname, const string& k, const string& v, const NONE_VALUE_CALLBACK& callback);

    void                                                            multiHget(const string& hname, const std::vector<std::string> &keys, const STRING_LIST_CALLBACK& callback);
    void                                                            multiHset(const string& hname, const std::unordered_map<std::string, std::string> &kvs, const NONE_VALUE_CALLBACK& callback);

    void                                                            zset(const std::string& name, const std::string& key, int64_t score,
                                                                        const NONE_VALUE_CALLBACK& callback);

    void                                                            zget(const std::string& name, const std::string& key, const ONE_INT64_CALLBACK& callback);

    void                                                            zsize(const std::string& name, const ONE_INT64_CALLBACK& callback);

    void                                                            zkeys(const std::string& name, const std::string& key_start, int64_t score_start, int64_t score_end,
                                                                        uint64_t limit, const STRING_LIST_CALLBACK& callback);

    void                                                            zscan(const std::string& name, const std::string& key_start, int64_t score_start, int64_t score_end,
                                                                        uint64_t limit, const STRING_LIST_CALLBACK& callback);

    void                                                            zclear(const std::string& name, const NONE_VALUE_CALLBACK& callback);

    void                                                            qpush(const std::string& name, const std::string& item, const NONE_VALUE_CALLBACK&);
    void                                                            qpop(const std::string& name, const ONE_STRING_CALLBACK&);
    void                                                            qslice(const std::string& name, int64_t begin, int64_t end, const STRING_LIST_CALLBACK& callback);
    void                                                            qclear(const std::string& name, const NONE_VALUE_CALLBACK& callback);

    void                                                            forceSyncRequest();
    void                                                            pull();
    void                                                            stopService();

private:
    /*Ͷ��û�з���ֵ��db����*/
    void                                                            pushNoneValueRequest(const char* request, int len, const NONE_VALUE_CALLBACK& callback);
    /*Ͷ�ݷ���ֵΪstring��db����*/
    void                                                            pushStringValueRequest(const char* request, int len, const ONE_STRING_CALLBACK& callback);
    /*Ͷ�ݷ���ֵΪstring list��db����*/
    void                                                            pushStringListRequest(const char* request, int len, const STRING_LIST_CALLBACK& callback);
    /*Ͷ�ݷ���ֵΪint64_t��db����*/
    void                                                            pushIntValueRequest(const char* request, int len, const ONE_INT64_CALLBACK& callback);

    parse_tree*                                                     processResponse(const string& response);
    void                                                            forgeError(const string& error, std::function<void(const string&)>& callback);

    void                                                            sendPing(DataSocket::PTR ds);
private:
    std::thread*                                                    mNetThread;
    int64_t                                                         mCallbackNextID;

    unordered_map<int64_t, ONE_STRING_CALLBACK>                     mOneStringValueCallback;
    unordered_map<int64_t, NONE_VALUE_CALLBACK>                     mNoValueCallback;
    unordered_map<int64_t, ONE_INT64_CALLBACK>                      mOntInt64Callback;
    unordered_map<int64_t, STRING_LIST_CALLBACK>                    mStringListCallback;

    bool                                                            mRunIOLoop;
    EventLoop                                                       mNetService;
    vector<DataSocket::PTR>                                         mProxyClients;

    /*Ͷ�ݵ������̵߳�db����*/
    struct RequestMsg
    {
        RequestMsg()
        {}

        RequestMsg(const std::function<void(const string&)>& acallback, const string& acontent) : callback(acallback), content(acontent)
        {
        }

        RequestMsg(std::function<void(const string&)>&& acallback, string&& acontent) : callback(std::move(acallback)), content(std::move(acontent))
        {
        }

        RequestMsg(RequestMsg &&a) : callback(std::move(a.callback)), content(std::move(a.content))
        {
        }

        RequestMsg & operator = (RequestMsg &&a)
        {
            if (this != &a)
            {
                callback = std::move(a.callback);
                content = std::move(a.content);
            }

            return *this;
        }
        std::function<void(const string&)> callback;    /*�û��̵߳��첽�ص�*/
        string  content;                                /*�����Э������*/
    };

    MsgQueue<RequestMsg>                                            mRequestList;
    MsgQueue<std::function<void(void)>>                             mLogicFunctorMQ;

    MyRequestProcotol*                                              mRequestProtocol;
    SSDBProtocolResponse*                                           mResponseProtocol;
};

#endif